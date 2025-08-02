/********************************************************
 * Description : cm256 codec
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Version     : 1.0
 * History     :
 * Copyright(C): 2025
 ********************************************************/

#ifdef _MSC_VER
#include <winsock2.h>
#else
#include <sys/time.h>
#include <arpa/inet.h>
#endif // _MSC_VER

#include <ctime>
#include <cstring>

#include "cm256.h"
#include "cm256_codec.h"

#pragma pack(push, 1)

struct block_header_t
{
    uint16_t            frame_index;
    uint8_t             frame_filter;
    uint8_t             block_index;
    uint8_t             original_count;
    uint8_t             recovery_count;
};

struct block_body_t
{
    uint16_t            block_bytes;
    char                block_chunk[1];
};

struct block_t
{
    block_header_t      header;
    block_body_t        body;
};

#pragma pack(pop)

frame_header_t::frame_header_t()
    : frame_index(0)
    , frame_filter(0)
    , original_count(0)
    , recovery_count(0)
    , block_count(0)
    , block_size(0)
    , block_bitmap()
{
    memset(block_bitmap, 0x0, sizeof(block_bitmap));
}

static void get_current_time(uint32_t & seconds, uint32_t & microseconds)
{
#ifdef _MSC_VER
    SYSTEMTIME sys_now = { 0x0 };
    GetLocalTime(&sys_now);
    seconds = static_cast<uint32_t>(time(nullptr));
    microseconds = static_cast<uint32_t>(sys_now.wMilliseconds * 1000);
#else
    struct timeval tv_now = { 0x0 };
    gettimeofday(&tv_now, nullptr);
    seconds = static_cast<uint32_t>(tv_now.tv_sec);
    microseconds = static_cast<uint32_t>(tv_now.tv_usec);
#endif // _MSC_VER
}

static bool create_original_blocks(CM256::cm256_block * blocks, std::list<std::vector<uint8_t>> & original_blocks, uint16_t frame_index, uint8_t frame_filter, uint8_t original_count, uint8_t recovery_count, uint16_t block_bytes, std::list<std::vector<uint8_t>>::const_iterator & iter)
{
    const uint16_t block_size = static_cast<uint16_t>(sizeof(block_header_t) + sizeof(uint16_t) + block_bytes);

    frame_index = htons(frame_index);

    for (uint8_t block_index = 0; block_index < original_count; ++block_index)
    {
        const std::vector<uint8_t> & data = *iter++;
        if (data.size() > block_bytes)
        {
            return false;
        }

        std::vector<uint8_t> original_buffer(block_size, 0x0);

        block_t * block = reinterpret_cast<block_t *>(&original_buffer[0]);
        block->header.frame_index = frame_index;
        block->header.frame_filter = frame_filter;
        block->header.block_index = block_index;
        block->header.original_count = original_count;
        block->header.recovery_count = recovery_count;
        if (!data.empty())
        {
            memcpy(block->body.block_chunk, &data[0], data.size());
        }
        block->body.block_bytes = htons(static_cast<uint16_t>(data.size()));

        blocks[block_index].Block = &block->body;
        blocks[block_index].Index = block_index;

        original_blocks.emplace_back(std::move(original_buffer));
    }

    return true;
}

static bool create_recovery_blocks(CM256::cm256_block * blocks, std::list<std::vector<uint8_t>> & recovery_blocks, uint16_t frame_index, uint8_t frame_filter, uint8_t original_count, uint8_t recovery_count, uint16_t block_bytes)
{
    if (0 == recovery_count)
    {
        return true;
    }

    uint8_t * recovery_data[256] = { 0x0 };

    const uint16_t block_size = static_cast<uint16_t>(sizeof(block_header_t) + sizeof(uint16_t) + block_bytes);

    frame_index = htons(frame_index);

    for (uint8_t block_index = 0; block_index < recovery_count; ++block_index)
    {
        std::vector<uint8_t> recovery_buffer(block_size, 0x0);

        block_t * block = reinterpret_cast<block_t *>(&recovery_buffer[0]);
        block->header.frame_index = frame_index;
        block->header.frame_filter = frame_filter;
        block->header.block_index = original_count + block_index;
        block->header.original_count = original_count;
        block->header.recovery_count = recovery_count;

        blocks[original_count + block_index].Block = &block->body;
        blocks[original_count + block_index].Index = original_count + block_index;

        recovery_data[block_index] = reinterpret_cast<uint8_t *>(&block->body);

        recovery_blocks.emplace_back(std::move(recovery_buffer));
    }

    CM256 cm256;
    if (!cm256.isInitialized())
    {
        return false;
    }

    CM256::cm256_encoder_params params = { original_count, recovery_count, static_cast<int>(sizeof(uint16_t) + block_bytes) };
    if (0 != cm256.cm256_encode(params, blocks, recovery_data))
    {
        return false;
    }

    return true;
}

bool cm256_encode(uint16_t & frame_index, uint8_t & frame_filter, std::list<std::vector<uint8_t>> & dst_data_list, const std::list<std::vector<uint8_t>> & src_data_list, double recovery_rate, std::size_t max_data_size, bool recovery_force)
{
    if (recovery_rate < 0.0 || recovery_rate >= 1.0)
    {
        return false;
    }

    if (0 == max_data_size)
    {
        for (std::list<std::vector<uint8_t>>::const_iterator iter = src_data_list.begin(); src_data_list.end() != iter; ++iter)
        {
            if (max_data_size < iter->size())
            {
                max_data_size = iter->size();
            }
        }
    }
    if (max_data_size >= 65536)
    {
        return false;
    }

    std::size_t data_list_left = src_data_list.size();
    std::list<std::vector<uint8_t>>::const_iterator iter = src_data_list.begin();

    uint16_t block_bytes = static_cast<uint16_t>(max_data_size);
    uint8_t original_count = static_cast<uint8_t>(255.0 * (1.0 - recovery_rate) + 0.5);
    uint8_t recovery_count = static_cast<uint8_t>(255 - original_count);

    while (0 != data_list_left)
    {
        if (original_count > data_list_left)
        {
            original_count = static_cast<uint8_t>(data_list_left);
            if (recovery_rate >= 1.0)
            {
                recovery_count = static_cast<uint8_t>(255 - data_list_left);
            }
            else
            {
                recovery_count = static_cast<uint8_t>(static_cast<double>(data_list_left) * recovery_rate / (1.0 - recovery_rate) + 0.5);
            }
        }
        if (recovery_force && recovery_rate > 0.0 && 0 == recovery_count)
        {
            recovery_count = 1;
        }
        data_list_left -= original_count;

        CM256::cm256_block blocks[256];

        std::list<std::vector<uint8_t>> original_blocks;
        if (!create_original_blocks(blocks, original_blocks, frame_index, frame_filter, original_count, recovery_count, block_bytes, iter))
        {
            return false;
        }

        std::list<std::vector<uint8_t>> recovery_blocks;
        if (!create_recovery_blocks(blocks, recovery_blocks, frame_index, frame_filter, original_count, recovery_count, block_bytes))
        {
            return false;
        }

        dst_data_list.splice(dst_data_list.end(), original_blocks);
        dst_data_list.splice(dst_data_list.end(), recovery_blocks);

        if (0 == ++frame_index)
        {
            ++frame_filter;
        }
    }

    return true;
}

static bool insert_frame_block(const void * data, std::size_t data_len, frames_t & frames, uint16_t & frame_index, uint32_t max_delay_microseconds)
{
    const block_t * block = reinterpret_cast<const block_t *>(data);
    const uint16_t block_size = static_cast<uint16_t>(data_len);

    const block_header_t & block_header = block->header;

    frame_index = ntohs(block_header.frame_index);

    frame_t & frame = frames.item[frame_index];
    frame_header_t & frame_header = frame.header;
    frame_body_t & frame_body = frame.body;

    if (0 == frame_header.block_count)
    {
        if (0 == frame_header.original_count || 
            block_size != frame_header.block_size || 
            block_header.frame_index != frame_header.frame_index || block_header.frame_filter != frame_header.frame_filter || 
            block_header.original_count != frame_header.original_count || block_header.recovery_count != frame_header.recovery_count)
        {
            frame_header.block_size = block_size;
            frame_header.frame_index = block_header.frame_index;
            frame_header.frame_filter = block_header.frame_filter;
            frame_header.original_count = block_header.original_count;
            frame_header.recovery_count = block_header.recovery_count;
            memset(frame_header.block_bitmap, 0x0, sizeof(frame_header.block_bitmap));
            frame_header.block_bitmap[block_header.block_index >> 3] |= (1 << (block_header.block_index & 7));
            if (block_header.block_index < block_header.original_count)
            {
                frame_body.original_list.emplace_back(std::vector<uint8_t>(reinterpret_cast<const uint8_t *>(block), reinterpret_cast<const uint8_t *>(block) + block_size));
            }
            else
            {
                frame_body.recovery_list.emplace_back(std::vector<uint8_t>(reinterpret_cast<const uint8_t *>(block), reinterpret_cast<const uint8_t *>(block) + block_size));
            }
            frame_header.block_count += 1;

            decode_timer_t decode_timer = { 0x0 };
            decode_timer.frame_index = frame_index;
            get_current_time(decode_timer.decode_seconds, decode_timer.decode_microseconds);
            decode_timer.decode_microseconds += max_delay_microseconds * frame_header.original_count;
            decode_timer.decode_seconds += decode_timer.decode_microseconds / 1000000;
            decode_timer.decode_microseconds %= 1000000;

            frames.decode_timer_list.push_back(decode_timer);

            return true;
        }
        else
        {
            return false;
        }
    }

    if (block_size != frame_header.block_size || 
        block_header.frame_index != frame_header.frame_index || block_header.frame_filter != frame_header.frame_filter || 
        block_header.original_count != frame_header.original_count || block_header.recovery_count != frame_header.recovery_count)
    {
        return false;
    }

    if (frame_header.block_bitmap[block_header.block_index >> 3] & (1 << (block_header.block_index & 7)))
    {
        return false;
    }

    if (frame_header.block_count == frame_header.original_count)
    {
        if (block_header.block_index >= block_header.original_count)
        {
            return false;
        }
        else
        {
            block_t * old_block = reinterpret_cast<block_t *>(&frame_body.recovery_list.back()[0]);
            frame_header.block_bitmap[old_block->header.block_index >> 3] &= ~(1 << (old_block->header.block_index & 7));
            frame_body.recovery_list.pop_back();
            frame_header.block_bitmap[block_header.block_index >> 3] |= (1 << (block_header.block_index & 7));
            frame_body.original_list.emplace_back(std::vector<uint8_t>(reinterpret_cast<const uint8_t *>(block), reinterpret_cast<const uint8_t *>(block) + block_size));
        }
    }
    else
    {
        frame_header.block_bitmap[block_header.block_index >> 3] |= (1 << (block_header.block_index & 7));
        if (block_header.block_index < block_header.original_count)
        {
            frame_body.original_list.emplace_back(std::vector<uint8_t>(reinterpret_cast<const uint8_t *>(block), reinterpret_cast<const uint8_t *>(block) + block_size));
        }
        else
        {
            frame_body.recovery_list.emplace_back(std::vector<uint8_t>(reinterpret_cast<const uint8_t *>(block), reinterpret_cast<const uint8_t *>(block) + block_size));
        }
        frame_header.block_count += 1;
    }

    return true;
}

static bool cm256_decode(frame_header_t & frame_header, frame_body_t & frame_body, std::list<std::vector<uint8_t>> & src_data_list)
{
    frame_header.block_count = 0;

    src_data_list.clear();
    src_data_list.swap(frame_body.original_list);

    if (!frame_body.recovery_list.empty())
    {
        if (src_data_list.size() + frame_body.recovery_list.size() == frame_header.original_count)
        {
            src_data_list.splice(src_data_list.end(), frame_body.recovery_list);

            CM256::cm256_block blocks[256];

            std::size_t block_index = 0;
            for (std::list<std::vector<uint8_t>>::iterator iter = src_data_list.begin(); src_data_list.end() != iter; ++iter)
            {
                std::vector<uint8_t> & data = *iter;
                block_t * block = reinterpret_cast<block_t *>(&data[0]);
                blocks[block_index].Block = &block->body;
                blocks[block_index].Index = block->header.block_index;
                ++block_index;
            }

            CM256 cm256;
            if (!cm256.isInitialized())
            {
                return false;
            }

            CM256::cm256_encoder_params params = { frame_header.original_count, frame_header.recovery_count, static_cast<int>(frame_header.block_size - sizeof(block_header_t)) };
            if (0 != cm256.cm256_decode(params, blocks))
            {
                return false;
            }
        }
        else
        {
            frame_body.recovery_list.clear();
        }
    }

    for (std::list<std::vector<uint8_t>>::iterator iter = src_data_list.begin(); src_data_list.end() != iter; ++iter)
    {
        std::vector<uint8_t> & data = *iter;
        block_t * block = reinterpret_cast<block_t *>(&data[0]);
        std::vector<uint8_t>(block->body.block_chunk, block->body.block_chunk + ntohs(block->body.block_bytes)).swap(data);
    }

    return true;
}

bool cm256_decode(const void * data, std::size_t data_len, frames_t & frames, std::list<std::vector<uint8_t>> & dst_data_list, uint32_t max_delay_microseconds, bool recovery_force)
{
    bool need_decode = recovery_force;

    if (nullptr != data && 0 != data_len)
    {
        uint16_t frame_index = 0;
        if (insert_frame_block(data, data_len, frames, frame_index, max_delay_microseconds))
        {
            const frame_t & frame = frames.item[frame_index];
            if (frame.header.block_count == frame.header.original_count)
            {
                need_decode = true;
            }
        }
    }
    else
    {
        need_decode = true;
    }

    if (need_decode)
    {
        uint32_t current_seconds = 0;
        uint32_t current_microseconds = 0;
        get_current_time(current_seconds, current_microseconds);
        std::list<decode_timer_t>::iterator iter = frames.decode_timer_list.begin();
        while (frames.decode_timer_list.end() != iter)
        {
            const decode_timer_t & decode_timer = *iter;
            frame_t & frame = frames.item[decode_timer.frame_index];
            if ((decode_timer.decode_seconds < current_seconds) || (decode_timer.decode_seconds == current_seconds && decode_timer.decode_microseconds < current_microseconds) || 
                (frame.header.block_count == frame.header.original_count))
            {
                std::list<std::vector<uint8_t>> src_data_list;
                cm256_decode(frame.header, frame.body, src_data_list);
                dst_data_list.splice(dst_data_list.end(), src_data_list);
                iter = frames.decode_timer_list.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    return true;
}
