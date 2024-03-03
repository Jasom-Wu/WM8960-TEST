//
// Created by Mr.WU on 2024/2/24.
//

#ifndef WM8960_TEST_WM8960APPS_H
#define WM8960_TEST_WM8960APPS_H
#include "stm32f1xx_hal.h"
#include "fatfs.h"

#define WAV_BUFFER_SIZE 10240
#define PLAY_SIZE 15000

//RIFF Chunk
typedef struct
{
    uint32_t ChunkID;       //CHUNK ID is "RIFF" = 0X46464952
    uint32_t ChunkSize ;    //ChunkSize = file_size - 8
    uint32_t Format;        //Format: "WAVE" = 0X45564157
}__packed ChunkRIFF;

//fmt Chunk
typedef struct
{
    uint32_t ChunkID;       //Chunk ID is "fmt" = 0X20746D66
    uint32_t ChunkSize ;    //ChunkSize(Not including ID and Size): 16.
    uint16_t AudioFormat;   //Audio Format = 0X01,Linear PCM;0X11 is IMA ADPCM
    uint16_t NumOfChannels; //channel number; 1,mono channel; 2,double channel;
    uint32_t SampleRate;    //SampleRate;0X1F40,8Khz
    uint32_t ByteRate;      //ByteRate;
    uint16_t BlockAlign;    //Chunk align (byte);
    uint16_t BitsPerSample; //Single sample data size; 4 bits is ADPCM,set as 4
//    uint16_t ByteExtraData; //extra data ; 2 ;linear PCM file don't have this.
}__packed ChunkFMT;

//fact Chunk
typedef struct
{
    uint32_t ChunkID;       //Chunk ID: "fact",0X74636166;Linear PCM don't have this.
    uint32_t ChunkSize ;    //Chunk Size(Not includeing ID and Size);This is 4.
    uint32_t FactSize;      //Transform to PCM File's size;
}__packed ChunkFACT;

//WAV File play control struct
typedef struct
{
    uint16_t audioformat;   //audio format;0X01 is Linear PCM;0X11 is IMA ADPCM.
    uint16_t nchannels;     //track number; 1,single track; 2,dual track;
    uint16_t blockalign;    //Chunk align (byte);
    uint32_t datasize;      //WAV data size;
    uint32_t totsec ;       //Play time (s);
    uint32_t cursec ;       //Current playback time (s)
    uint32_t bitrate;       //Bit rate (bit speed)
    uint32_t samplerate;    //sampling rate
    uint16_t bps;           //bit numbers,such as 16bit,24bit,32bit
    uint32_t datastart;     //Starting position of Data frame(Offset in the file)
}__packed wavctrl;

//Data Chunk
typedef struct
{
    uint32_t ChunkID;       //chunk ID: "data" = 0X5453494C
    uint32_t ChunkSize ;    //Chunk Size(Not includeing ID and Size)
}__packed ChunkDATA;

//I2S CallBack function Flag
typedef enum
{
    I2S_No_CallBack = 0,
    I2S_Half_Callback = 1,
    I2S_Callback = 2
}I2S_CallBack_Flag;

enum {
    AUDIO_NONE=0,
    AUDIO_PRE_PLAY,
    AUDIO_PLAY,
    AUDIO_RECORD,
    AUDIO_PAUSE,
    AUDIO_RESUME,
    AUDIO_CANCEL,
    AUDIO_END,
};

//WAV Header
typedef struct
{
    ChunkRIFF riff;     //RIFF Chunk
    ChunkFMT fmt;       //fmt Chunk
    //ChunkFACT fact;   //fact Chunk; This is not in Line PCM file
    ChunkDATA data;     //data Chunk
}__packed __WaveHeader;


extern volatile uint8_t audio_play_request;
extern volatile uint8_t audio_play_state;
extern volatile uint8_t audio_rec_request;
extern volatile uint8_t audio_rec_state;
extern FIL WAV_File;
extern volatile I2S_CallBack_Flag I2S_Flag;
extern char Play_List[10][40];
extern uint8_t Music_Num_MAX;
extern uint8_t Music_Num;

FRESULT ScanWavefiles(char* path);
uint8_t PlayWaveFile(char *pname);
uint8_t RecordWaveFile(char *fname,uint32_t fs);
uint8_t Get_WAV_Message(char* fname,wavctrl* wavx);
uint32_t Fill_WAV_Buffer(uint8_t *BUFF, uint16_t size);
void Volum_Control(uint8_t ctrl);
#endif //WM8960_TEST_WM8960APPS_H
