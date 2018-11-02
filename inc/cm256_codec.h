/********************************************************
 * Description : cm256 codec
 * Author      : ryan
 * Email       : ryan@rayvision.com
 * Version     : 1.0
 * History     :
 * Copyright(C): RAYVISION
 ********************************************************/

#ifndef CM256_CODEC_H
#define CM256_CODEC_H


#ifdef _MSC_VER
    #define CM256_CODEC_CDECL            __cdecl
    #ifdef EXPORT_CM256_CODEC_DLL
        #define CM256_CODEC_TYPE         __declspec(dllexport)
    #else
        #ifdef USE_CM256_CODEC_DLL
            #define CM256_CODEC_TYPE     __declspec(dllimport)
        #else
            #define CM256_CODEC_TYPE
        #endif // USE_CM256_CODEC_DLL
    #endif // EXPORT_CM256_CODEC_DLL
#else
    #define CM256_CODEC_CDECL
    #define CM256_CODEC_TYPE
#endif // _MSC_VER

#define CM256_CODEC_CXX_API(return_type) extern CM256_CODEC_TYPE return_type CM256_CODEC_CDECL

#include <cstdint>
#include <cstring>
#include <map>
#include <list>
#include <vector>

struct CM256_CODEC_TYPE frame_header_t
{
    uint16_t                            frame_index;
    uint8_t                             frame_filter;
    uint8_t                             original_count;
    uint8_t                             recovery_count;
    uint8_t                             block_count;
    uint16_t                            block_size;
    uint8_t                             block_bitmap[32];

    frame_header_t();
};

struct frame_body_t
{
    std::list<std::vector<uint8_t>>     original_list;
    std::list<std::vector<uint8_t>>     recovery_list;
};

struct frame_t
{
    frame_header_t                      header;
    frame_body_t                        body;
};

struct decode_timer_t
{
    uint16_t                            frame_index;
    uint32_t                            decode_seconds;
    uint32_t                            decode_microseconds;
};

struct frames_t
{
    std::map<uint16_t, frame_t>         item;
    std::list<decode_timer_t>           decode_timer_list;
};


CM256_CODEC_CXX_API(bool)
cm256_encode(
    uint16_t & frame_index, 
    uint8_t & frame_filter, 
    std::list<std::vector<uint8_t>> & dst_data_list, 
    const std::list<std::vector<uint8_t>> & src_data_list, 
    double recovery_rate, 
    std::size_t max_data_size = 0, 
    bool recovery_force = false
);

CM256_CODEC_CXX_API(bool)
cm256_decode(
    const void * data, 
    std::size_t data_len, 
    frames_t & frames, 
    std::list<std::vector<uint8_t>> & dst_data_list, 
    uint32_t max_delay_microseconds = 1000 * 15, 
    bool recovery_force = false
);


#endif // CM256_CODEC_H
