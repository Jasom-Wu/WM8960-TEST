//
// Created by Mr.WU on 2024/2/24.
//

#include "WM8960Apps.h"
#include "string.h"
#include "WM8960.h"
#include "i2s.h"
#include "stdio.h"
#include "simple_gpio.h"




FIL WAV_File;             //File struct

volatile wavctrl WaveCtrlData;     //Play control struct
uint8_t WAV_Buffer[WAV_BUFFER_SIZE];  //The buffer to ache WAV data
uint8_t TempBuf[WAV_BUFFER_SIZE/2];
volatile I2S_CallBack_Flag I2S_Flag; //I2S CallBack function flag.


/* ============Globals of Play===============*/
volatile uint8_t audio_play_request = AUDIO_NONE;
volatile uint8_t audio_play_state = AUDIO_NONE;

volatile uint32_t WAV_LastData;    //The size last data to be played


char Play_List[10][40] = {0}; //play list
uint8_t Music_Num_MAX;    //Number of music.
uint8_t Music_Num = 0;    //the number of music which is being played.

/* ============Globals of Record=============*/
volatile uint8_t audio_rec_request = AUDIO_NONE;
volatile uint8_t audio_rec_state = AUDIO_NONE;

volatile __WaveHeader WAV_Header;  //The file head of WAV file
volatile uint32_t RecDataSize;
volatile uint32_t WriteSize;       //The byte numbers of writing to file every time.



/**
  * @brief  Scan the WAV files that set the path.
  * @param  path: Path to scan.
  * @retval None
  */

FRESULT ScanWavefiles(char* path) {

  FRESULT res;
  FILINFO fno;
  DIR dir;
  uint16_t i,j;

  res = f_opendir(&dir, path);    //Open the directory
  if(res != FR_OK)  {
    printf("f_opendir error !\r\n");
    return res;
  }

  for(i=0;;i++) {                 //Scan the files in the directory
    res = f_readdir(&dir, &fno);  //read a item
    if(res != FR_OK)  {
      printf("f_readdir error !\r\n");
      return res;
    }
    if(fno.fname[0] == 0)         //scan to the end of the path
      break;

    for(j=0;j<_MAX_LFN;j++) {
      if(fno.fname[j] == '.')     //Check if the type of the file is WAV
        break;
    }

    if(((fno.fname[j+1] == 'w')||(fno.fname[j+1] == 'W'))
       &&((fno.fname[j+2] == 'a')||(fno.fname[j+2] == 'A'))
       &&((fno.fname[j+3] == 'v')||(fno.fname[j+3] == 'V'))) //The file is WAV
    {
      strcpy(Play_List[i], path);     //Copy type of file is WAV
      strcat(Play_List[i],"/");       // Add '/' to the buffer
      strcat(Play_List[i],fno.fname); // Add file name to the buffer
      printf("%s\r\n", Play_List[i]); // print the whole file path to the UART
    }
  }
  res = f_closedir(&dir);             //Close the directory
  if(res != FR_OK)  {
    printf("f_closedir error !\r\n");
    return res;
  }

  Music_Num_MAX = i;

  printf("Scan WAV Files complete ! Music_Num = %d\r\n",Music_Num_MAX);

  return res;
}



/**
  * @brief  Open the WAV file, get the message of the file.
  * @param  fname: name of the file you want to get its massage.
  * @param  wavx: the struct of data control.
  * @retval None
  */
uint8_t Get_WAV_Message(char* fname, wavctrl* wavx) {

  uint8_t res = 0;
  uint32_t br = 0;

  ChunkRIFF *riff;
  ChunkFMT *fmt;
  ChunkFACT *fact;
  ChunkDATA *data;

  res = f_open(&WAV_File, (TCHAR *)fname, FA_READ);     //Open the file
  if(res == FR_OK) {

    f_read(&WAV_File, TempBuf, WAV_BUFFER_SIZE/2, &br); //Read WAV_BUFFER_SIZE/2 bytes data

    riff = (ChunkRIFF *)TempBuf;      //Get RIFF Chunk

    if(riff->Format == 0x45564157)  { //Format = "WAV"

      fmt = (ChunkFMT *)(TempBuf+12); //Get FMT Chunk
      if(fmt->AudioFormat==1||fmt->AudioFormat==3)        //Linear PCM or 32 bits WAVE
      {
        fact=(ChunkFACT *)(TempBuf+12+8+fmt->ChunkSize);  //Read FACT chunk

        if((fact->ChunkID == 0x74636166)||(fact->ChunkID==0X5453494C))
          wavx->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;  //When there is fact/LIST Chunk.
        else
          wavx->datastart=12+8+fmt->ChunkSize;
        data = (ChunkDATA *)(TempBuf+wavx->datastart);
        if(data->ChunkID==0X61746164) {           //Read DATA Chunk success
          wavx->audioformat=fmt->AudioFormat;     //Audio Format
          wavx->nchannels=fmt->NumOfChannels;     //channel number
          wavx->samplerate=fmt->SampleRate;				//Sample Rate
          wavx->bitrate=fmt->ByteRate*8;					//Byte Rate
          wavx->blockalign=fmt->BlockAlign;				//Block Align
          wavx->bps=fmt->BitsPerSample;						//number of chunk, 8/16/24/32 bits
          wavx->datasize=data->ChunkSize;					//Size of audio data chunk
          wavx->datastart=wavx->datastart+8;			//data stream start offest
          printf("WAV.audioformat:%d\r\n",wavx->audioformat);
          printf("WAV.nchannels:%d\r\n",wavx->nchannels);
          printf("WAV.samplerate:%d\r\n",wavx->samplerate);
          printf("WAV.bitrate:%d\r\n",wavx->bitrate);
          printf("WAV.blockalign:%d\r\n",wavx->blockalign);
          printf("WAV.bps:%d\r\n",wavx->bps);
          printf("WAV.datasize:%d\r\n",wavx->datasize);
          printf("WAV.datastart:%d\r\n",wavx->datastart);
        }
        else  {
          printf("Not find data chunk !!\r\n");
          printf("data->ChunkID = 0x%x\r\n",data->ChunkID);
          res = 4;
        }
      }
      else  {
        printf("Not linear PCM, not support !!\r\n");
        res = 3;
      }
    }
    else  {
      printf("Not WAV file !!\r\n");
      res = 2;
    }
  }
  else  {
    printf("Get_WAV_Message.f_open error !!\r\n");
    res = 1;
  }
  return res;
}

/**
  * @brief  Open the WAV file, get the message of the file.
  * @param  BUFF: the pointer of the buffer to cached data.
  * @param  size: the byte mumber of data.
  * @retval None
  */
uint32_t Fill_WAV_Buffer(uint8_t *BUFF, uint16_t size) {
  uint32_t NeedReadSize=0;
  uint32_t ReadSize;
  uint32_t i;
  uint8_t *p;
  float *f;
  int sound;


  if(WaveCtrlData.nchannels==2) {
    if(WaveCtrlData.bps == 16)          //16-bit audio,read data directly
    {
      f_read(&WAV_File,BUFF,size,(UINT*)&ReadSize);
    }
    else if(WaveCtrlData.bps==24)       //24-bit audio, adjust the order between the read data and the DMA cache
    {
      printf("WaveCtrlData.bps = %d\r\n",WaveCtrlData.bps);
      NeedReadSize=(size/4)*3;                                  //Number of bytes to read
      f_read(&WAV_File,TempBuf,NeedReadSize,(UINT*)&ReadSize);	//Read data
      p=TempBuf;
      ReadSize=(ReadSize/3)*4;                                  //Size of data after fill
      //printf("%d\r\n",ReadSize);
      for(i=0;i<ReadSize;)
      {
        BUFF[i]=p[0];
        BUFF[i+1]=p[1];
        BUFF[i+2]=p[2];
        BUFF[i+3]=0;

        i+=4;
        p+=3;
      }
    }
    else if(WaveCtrlData.bps == 8)      //8-bit audio, data need to be transformed to 16-bit mode before play
    {
      printf("WaveCtrlData.bps = %d\r\n",WaveCtrlData.bps);
      NeedReadSize=size/2;                                      //Number of bytes to read
      f_read(&WAV_File,TempBuf,NeedReadSize,(UINT*)&ReadSize);	//Read data
      p=TempBuf;
      ReadSize=ReadSize*2;                                      //Size of data after fill
      for(i=0;i<ReadSize;)
      {
        BUFF[i]=0;
        BUFF[i+1]=*p+0x80;
        p++;
        i=i+2;
      }
    }
    else if(WaveCtrlData.bps == 32)     //32bit WAVE, floating-point numbers [(-1) ~ 1] to represent sound
    {
      printf("WaveCtrlData.bps = %d\r\n",WaveCtrlData.bps);
      f_read(&WAV_File,TempBuf,size,(UINT*)&ReadSize);					//Read data
      f=(float*)TempBuf;
      for(i=0;i<ReadSize;)
      {
        //printf("f=%f\r\n",*f);
        sound=0x7FFFFFFF*(*f);
        BUFF[i]=(uint8_t)(sound>>16);
        BUFF[i+1]=(uint8_t)(sound>>24);
        BUFF[i+2]=(uint8_t)(sound);
        BUFF[i+3]=(uint8_t)(sound>>8);
        f++;
        i=i+4;
      }
    }
    else  {
      printf("WaveCtrlData.bps = %d\r\n",WaveCtrlData.bps);
      printf("Error !!\r\n");
    }
  }
    //Signal channel，adjust to dual channel data for playback
  else
  {
    if(WaveCtrlData.bps==16)
    {
      NeedReadSize=size/2;                                      //Number of bytes to read
      f_read(&WAV_File,TempBuf,NeedReadSize,(UINT*)&ReadSize);	//Read data
      p=TempBuf;
      ReadSize=ReadSize*2;                                      //Size of data after fill
      for(i=0;i<ReadSize;)
      {
        BUFF[i]=p[0];
        BUFF[i+1]=p[1];
        BUFF[i+2]=p[0];
        BUFF[i+3]=p[1];
        i+=4;
        p=p+2;
      }
    }
    else if(WaveCtrlData.bps==24)																	//24-bit audio
    {
      NeedReadSize=(size/8)*3;                                  //Number of bytes to read
      f_read(&WAV_File,TempBuf,NeedReadSize,(UINT*)&ReadSize);  //Read data
      p=TempBuf;
      ReadSize=(ReadSize/3)*8;                                  //Size of data after fill
      for(i=0;i<ReadSize;)
      {
        BUFF[i]=p[1];
        BUFF[i+1]=p[2];
        BUFF[i+2]=0;
        BUFF[i+3]=p[0];
        BUFF[i+4]=p[1];
        BUFF[i+5]=p[2];
        BUFF[i+6]=0;
        BUFF[i+7]=p[0];
        p+=3;
        i+=8;
      }
    }
    else if(WaveCtrlData.bps==8)                                //8-bit audio
    {
      NeedReadSize=size/4;                                      //Number of bytes to read
      f_read(&WAV_File,TempBuf,NeedReadSize,(UINT*)&ReadSize);  //Read data
      p=TempBuf;
      ReadSize=ReadSize*4;                                      //Size of data after fill
      for(i=0;i<ReadSize;)
      {
        BUFF[i]=0;
        BUFF[i+1]=*p+0x80;
        BUFF[i+2]=0;
        BUFF[i+3]=*p+0x80;
        i+=4;
        p++;
      }
    }
    else                                                        //32-bit audio
    {
      NeedReadSize=size/2;                                      //Number of bytes to read
      f_read(&WAV_File,TempBuf,NeedReadSize,(UINT*)&ReadSize);	//Read data
      f=(float*)TempBuf;
      ReadSize=ReadSize*2;                                      //Size of data after fill
      for(i=0;i<ReadSize;)
      {
        sound=0x7FFFFFFF*(*f);
        BUFF[i+4] = BUFF[i]   = (uint8_t)(sound>>16);
        BUFF[i+5] = BUFF[i+1] = (uint8_t)(sound>>24);
        BUFF[i+6] = BUFF[i+2] = (uint8_t)(sound);
        BUFF[i+7] = BUFF[i+3] = (uint8_t)(sound>>8);
        f++;
        i=i+8;
      }
    }
  }
  if(ReadSize<size)   //Data is not enough, supplementary '0'
  {
    for(i=ReadSize;i<size-ReadSize;i++)
      BUFF[i] = 0;
  }
  f_sync(&WAV_File);
  return ReadSize;
}


void IIS_FreqModify(uint32_t samplerate){
  if(samplerate == hi2s2.Init.AudioFreq)
    return;
  hi2s2.Init.AudioFreq = samplerate;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
}
void IIS_ModeModify(uint32_t mode){
  if(mode == hi2s2.Init.Mode)
    return;
  hi2s2.Init.Mode = mode;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  if(mode == I2S_MODE_MASTER_RX){
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  }
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
uint8_t PlayWaveFile(char *pname){
  uint8_t res;
  if (audio_play_request != AUDIO_PLAY){
    audio_play_request = AUDIO_NONE;
    return 0xff;
  }
  res = WM89060_Init(WM8960_MODE_DAC_ENABLE);
  res += Get_WAV_Message(pname,&WaveCtrlData);    //Get the messages of the WAV file
  if(res != 0){
    f_close(&WAV_File);
    audio_play_request = AUDIO_NONE;
    return res;
  }
  PBout(5) = 1;
  PBout(1) = 0;

  audio_play_state = AUDIO_PLAY;
  I2S_Flag = I2S_No_CallBack;
  printf("Now Play: %s\r\n",pname);
  WAV_LastData = WaveCtrlData.datasize;
  IIS_ModeModify(I2S_MODE_MASTER_TX);
  IIS_FreqModify(WaveCtrlData.samplerate);
  f_lseek(&WAV_File, WaveCtrlData.datastart);
  Fill_WAV_Buffer(WAV_Buffer, WAV_BUFFER_SIZE);
  HAL_I2S_Transmit_DMA(&hi2s2,(uint16_t*)WAV_Buffer, WAV_BUFFER_SIZE/2);
  while(audio_play_state != AUDIO_CANCEL && audio_play_state != AUDIO_END){
    if(audio_play_state == AUDIO_PLAY){
      if(I2S_Flag == I2S_Half_Callback) {
        Fill_WAV_Buffer(WAV_Buffer,WAV_BUFFER_SIZE/2);
        WAV_LastData -= WAV_BUFFER_SIZE/2;
        I2S_Flag = I2S_No_CallBack;
      }
      else if(I2S_Flag == I2S_Callback) {
        Fill_WAV_Buffer((WAV_Buffer+WAV_BUFFER_SIZE/2),WAV_BUFFER_SIZE/2);
        WAV_LastData -= WAV_BUFFER_SIZE/2;
        I2S_Flag = I2S_No_CallBack;
      }
    }
    if(audio_play_state == AUDIO_PLAY && audio_play_request == AUDIO_PAUSE){
      HAL_I2S_DMAPause(&hi2s2); //Pause DMA data flow
      audio_play_state = AUDIO_PAUSE;
      PBout(5) = 0;
      PBout(1) = 1;
    }
    if(audio_play_state == AUDIO_PAUSE && audio_play_request == AUDIO_RESUME){
      HAL_I2S_DMAResume(&hi2s2);
      audio_play_state = AUDIO_PLAY;
      PBout(5) = 1;
      PBout(1) = 0;
    }
    if(audio_play_request == AUDIO_CANCEL){
      audio_play_state = AUDIO_CANCEL;
    }
  }
  HAL_I2S_DMAStop(&hi2s2);
  if(audio_play_request == AUDIO_END){
    HAL_I2S_Transmit(&hi2s2,(uint16_t*)WAV_Buffer, WAV_LastData/2,0xffff);
  }
  f_close(&WAV_File);
  audio_play_request = AUDIO_NONE;
  PBout(5) = 1;
  PBout(1) = 1;
  return res;
}


/**
  * @brief  Initialize WAV file
  * @param  fname: name of file(exclude suffix)
  *         fs: samplerate of wav record file
  * @retval None
  */
uint8_t WAV_File_Init(char *fname,uint32_t fs) {

  FRESULT Res;
  uint16_t i;
  WAV_Header.riff.ChunkID=0X46464952;                   //"RIFF"
  WAV_Header.riff.ChunkSize=0;                          //Not yet determined, final calculation required
  WAV_Header.riff.Format=0X45564157;                    //"WAVE"
  WAV_Header.fmt.ChunkID=0X20746D66;                    //"fmt "
  WAV_Header.fmt.ChunkSize=16;                          //18 bytes
  WAV_Header.fmt.AudioFormat=0X01;                      //0X01, lineal PCM;0X03, IMA ADPCM
  WAV_Header.fmt.NumOfChannels=2;                       //dual channel
  WAV_Header.fmt.SampleRate=fs;                         //Custom sampling rate
  WAV_Header.fmt.ByteRate=WAV_Header.fmt.SampleRate*4;  //Byte rate = sampling rate * number of channels * (ADC bit number/8)
  WAV_Header.fmt.BlockAlign=4;                          //Chunk size = channel number *(ADC /8)
  WAV_Header.fmt.BitsPerSample=16;                      //16-bit PCM
  WAV_Header.data.ChunkID=0X61746164;                    //"data"
  WAV_Header.data.ChunkSize=0;                          //Data size, final calculation required

  if(fname==NULL){
    for(i=0;i<100;i++)  {
      sprintf(TempBuf,"0:/REC/%d.wav",i);
      printf("%s\r\n",TempBuf);
      Res=f_open(&WAV_File,TempBuf,FA_CREATE_NEW | FA_WRITE);

      if(Res==FR_EXIST)
        continue;
      else if(Res==FR_OK)  {
        Res=f_write(&WAV_File,(const void*)(&WAV_Header),sizeof(__WaveHeader),&WriteSize);//write file head
        printf("File_Header f_write : %d\r\n", Res);
        break;
      }
      else  {
        printf("f_open: %d\r\n",Res);
        Res = 1;   //Create new file error
      }
    }
  }
  else{
    sprintf(TempBuf,"0:/REC/%s.wav",fname);
    Res=f_open(&WAV_File,TempBuf,FA_CREATE_ALWAYS | FA_WRITE);
    if(Res==FR_OK)  {
      //CloseFileFlag=1;
      Res=f_write(&WAV_File,(const void*)(&WAV_Header),sizeof(__WaveHeader),&WriteSize);//write file head
      printf("File_Header f_write : %d\r\n", Res);
    }
    else{
      printf("f_open: %d\r\n",Res);
      Res = 1;
    }
  }
  if(Res){
    f_close(&WAV_File);
    f_unlink(TempBuf);
  }
  return Res;       //Create new file completed
}


uint8_t RecordWaveFile(char *fname,uint32_t fs){
  uint8_t res;
  if(audio_rec_request != AUDIO_RECORD){
    audio_rec_request = AUDIO_NONE;
    return 0xee;
  }
  res = WM89060_Init(WM8960_MODE_ADC_ENABLE);
  res += WAV_File_Init(fname,fs);
  if(res != 0){
    audio_rec_request = AUDIO_NONE;
    return res;
  }

  IIS_ModeModify(I2S_MODE_MASTER_RX);
  IIS_FreqModify(fs);
  audio_rec_state = AUDIO_RECORD;
  I2S_Flag = I2S_No_CallBack;
  RecDataSize = 0;//clear to zero!!
  HAL_I2S_Receive_DMA(&hi2s2,(uint16_t *) WAV_Buffer, WAV_BUFFER_SIZE/2);
  PBout(5) = 1;
  PBout(0) = 0;
  while(audio_rec_state != AUDIO_CANCEL && audio_rec_state != AUDIO_END){
    if(audio_rec_state == AUDIO_RECORD){
      if(I2S_Flag == I2S_Half_Callback) {
        res = f_write(&WAV_File,WAV_Buffer,WAV_BUFFER_SIZE/2,&WriteSize);
        RecDataSize += WriteSize;
        I2S_Flag = I2S_No_CallBack;
      }
      else if(I2S_Flag == I2S_Callback) {
        res = f_write(&WAV_File,(WAV_Buffer+(WAV_BUFFER_SIZE/2)),WAV_BUFFER_SIZE/2,&WriteSize);
        RecDataSize += WriteSize;
        I2S_Flag = I2S_No_CallBack;
      }
    }
    if(audio_rec_state == AUDIO_RECORD && audio_rec_request == AUDIO_PAUSE){
      HAL_I2S_DMAPause(&hi2s2); //Pause DMA data flow
      audio_rec_state = AUDIO_PAUSE;
      PBout(5) = 0;
      PBout(0) = 1;
    }
    if(audio_rec_state == AUDIO_PAUSE && audio_rec_request == AUDIO_RESUME){
      HAL_I2S_DMAResume(&hi2s2);
      audio_rec_state = AUDIO_RECORD;
      PBout(5) = 1;
      PBout(0) = 0;
    }
    if(audio_rec_request == AUDIO_CANCEL){
      audio_rec_state = AUDIO_CANCEL;
    }
    if(audio_rec_request == AUDIO_END){
      audio_rec_state = AUDIO_END;
      ScanWavefiles("0:/REC");
    }
  }
  if(audio_rec_state == AUDIO_END){
    HAL_I2S_DMAStop(&hi2s2);
    audio_rec_request = AUDIO_NONE;
    WAV_Header.riff.ChunkSize = RecDataSize+36;   //File size - 8;
    WAV_Header.data.ChunkSize = RecDataSize;      //Data size
    printf("RecDataSize=%d\r\n",RecDataSize);
    res += f_sync(&WAV_File);
    res += f_lseek(&WAV_File,0);										  //offset to file head.
    res += f_write(&WAV_File,&WAV_Header,sizeof(__WaveHeader),&WriteSize);  //write file head
  }
  res += f_close(&WAV_File);
  if(audio_rec_state == AUDIO_CANCEL)
    f_unlink(TempBuf);
  PBout(5) = 1;
  PBout(0) = 1;
  return res;
}


void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)  {
  I2S_Flag = I2S_Half_Callback;
}
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s){
  I2S_Flag = I2S_Half_Callback;
}
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  /* Manage the remaining file size and new address offset: */

  /* Check if the end of file has been reached */
    if(WAV_LastData >= WAV_BUFFER_SIZE)  {
      I2S_Flag = I2S_Callback;
      HAL_I2S_Transmit_DMA(hi2s,(uint16_t *)WAV_Buffer, WAV_BUFFER_SIZE/2);
    }
    else  {
      audio_play_state = AUDIO_END;
    }
}
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s){
  I2S_Flag = I2S_Callback;
  if(audio_rec_state == AUDIO_RECORD)
    HAL_I2S_Receive_DMA(hi2s,(uint16_t *)WAV_Buffer, WAV_BUFFER_SIZE/2);
}