/********************************************************
 * Description : cm256 codec test
 * Author      : ryan
 * Email       : ryan@rayvision.com
 * Version     : 1.0
 * History     :
 * Copyright(C): RAYVISION
 ********************************************************/

#include <ctime>
#include <algorithm>
#include "cm256_codec.h"

int main()
{
    srand(static_cast<uint32_t>(time(0)));

    std::list<std::vector<uint8_t>> src_data_list;
    for (std::size_t i = 0; i < 400; ++i)
    {
        uint8_t buffer[1600] = { 0x0 };
        for (std::size_t j = 0; j < sizeof(buffer); ++j)
        {
            buffer[j] = static_cast<uint8_t>(rand() % 256);
        }
        src_data_list.push_back(std::vector<uint8_t>(buffer, buffer + 1001 + rand() % 600));
    }

    std::list<std::vector<uint8_t>> tmp_data_list;

    uint16_t frame_index = 0;
    uint8_t frame_filter = 0;
    if (!cm256_encode(frame_index, frame_filter, tmp_data_list, src_data_list, 0.1, 0, true))
    {
        return (1);
    }

    std::list<std::vector<uint8_t>>::iterator iter_1_b = tmp_data_list.begin();
    std::advance(iter_1_b, 205);
    std::list<std::vector<uint8_t>>::iterator iter_1_e = iter_1_b;
    std::advance(iter_1_e, 25);

    std::list<std::vector<uint8_t>>::iterator iter_2_b = iter_1_e;
    std::advance(iter_2_b, 25);
    std::list<std::vector<uint8_t>>::iterator iter_2_e = iter_2_b;
    std::advance(iter_2_e, 19);

    tmp_data_list.erase(iter_1_b, iter_1_e);
    tmp_data_list.erase(iter_2_b, iter_2_e);

    if (tmp_data_list.size() != src_data_list.size())
    {
        return (2);
    }

    std::list<std::vector<uint8_t>> dst_data_list;

    frames_t frames;
    for (std::list<std::vector<uint8_t>>::const_iterator iter = tmp_data_list.begin(); tmp_data_list.end() != iter; ++iter)
    {
        const std::vector<uint8_t> & data = *iter;
        if (!cm256_decode(&data[0], data.size(), frames, dst_data_list, 1000 * 15, false))
        {
            return (3);
        }
    }

    if (src_data_list.size() != dst_data_list.size())
    {
        return (4);
    }

    for (std::list<std::vector<uint8_t>>::const_iterator iter = src_data_list.begin(); src_data_list.end() != iter; ++iter)
    {
        if (dst_data_list.end() == std::find(dst_data_list.begin(), dst_data_list.end(), *iter))
        {
            return (5);
        }
    }

    for (std::list<std::vector<uint8_t>>::const_iterator iter = dst_data_list.begin(); dst_data_list.end() != iter; ++iter)
    {
        if (src_data_list.end() == std::find(src_data_list.begin(), src_data_list.end(), *iter))
        {
            return (6);
        }
    }

    if (dst_data_list == src_data_list)
    {
        printf("ok 1\n");
    }
    else
    {
        printf("ok 2\n");
    }

    return (0);
}
