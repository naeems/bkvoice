
#include "stdio.h"
#include "wb_vad.h"

#include <winsock2.h> 
#include <windows.h>
#include "config.h"
#include "record.h"
#include "play.h"

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"user32.lib")

#define MAX_SEM_COUNT 10

HANDLE ghSemaphore;//�ź���

HANDLE hRecord, hPlay, hUDPSend, hUDPRecv;
HANDLE eventRecord, eventPlay, eventUDPSend, eventUDPRecv;
DWORD threadRecord, threadPlay, threadUDPSend, threadUDPRecv;

static FILE *logFile;

typedef struct tagAudioBuf AUDIOBUF, *pAUDIOBUF;

//��Ƶ�ɼ����Žṹ
struct tagAudioBuf
{
	//char valid;
	char recordvalid;//¼����Ч
	char recvvalid;//
	pAUDIOBUF pPrior;//ǰָ��
	pAUDIOBUF pNext;//��ָ��
	int count;//��Ч��������
	//short data[SIZE_AUDIO_FRAME/2];
	//char *data;
    char data[512];
	unsigned int frameNO;//֡���
	DWORD time;//ʱ��
#if SPEEX_ENABLED	
	char speexencodevalid;//speex�����������Ƿ���Ч
	char speexdecodevalid;//speex�����������Ƿ���Ч
	char speexdata[SIZE_AUDIO_FRAME];//speex���������
	short datadecode[SIZE_AUDIO_FRAME];//speex���������
#endif
};

AUDIOBUF buffers[BUFCOUNT];
pAUDIOBUF pHeaderPut;//����������ͷ
pAUDIOBUF pHeaderGet;//����������ͷ

#if 1
#define RECORD_FILE_ENABLED   0

static int frameNO = 0;
//??????
DWORD WINAPI voice_record_thread_runner(LPVOID lpParam)
{
	HANDLE eventRecord = (HANDLE)(lpParam);
	MSG   msg;
	LPWAVEHDR lpHdr;

	int frame=0; 
	//float indata[2056];
	//VadVars *vadstate;
#if RECORD_FILE_ENABLED
        TMemoryStream * stream = new TMemoryStream();
#endif                
	//wb_vad_init(&(vadstate));			//vad???
	if(openMicAndStartRecording(GetCurrentThreadId()) < 0) return -1;

	while(GetMessage(&msg, 0, 0, 0))
	{
		if(WaitForSingleObject(eventRecord, 1)==WAIT_OBJECT_0)
		{
			break;
		}

		switch(msg.message)
		{
			case MM_WIM_DATA:				
				lpHdr = (LPWAVEHDR)msg.lParam;
				waveInUnprepareHeader(getRecordHandler(), lpHdr, sizeof(WAVEHDR));

				if(lpHdr->lpData!=NULL )
				{

                                /*
                                        for(int i=0;i<dwSample/1000*SAMPLINGPERIOD*wChannels;i++)
                                        {
                                                indata[i] = (float)(((short*)(lpHdr->lpData))[i]);
                                        }
                                */
                                        /*
                                        if(wb_vad(vadstate,indata) == 1)
                                        {
                                        //memcpy(&(pHeaderPut->data[0]), (short*)(lpHdr->lpData), dwSample/1000*SAMPLINGPERIOD*2*wChannels);

                                        }
                                        */

#if RECORD_FILE_ENABLED
                                        stream->Write((short*)(lpHdr->lpData), dwSample/1000*SAMPLINGPERIOD*2*wChannels);
#endif
                        memcpy(&(pHeaderPut->data[0]), (short*)(lpHdr->lpData), dwSample/1000*SAMPLINGPERIOD*2*wChannels);
						pHeaderPut->frameNO = frameNO++;
						pHeaderPut->time = timeGetTime();
                        pHeaderPut->recordvalid = TRUE;
                        pHeaderPut = pHeaderPut->pNext;

						if (!ReleaseSemaphore( 
							ghSemaphore,  // handle to semaphore - hSemaphore��Ҫ���ӵ��ź������
							1,            // increase count by one - lReleaseCount�����ӵļ���
							NULL) )       // not interested in previous count - lpPreviousCount������ǰ����ֵ����
						{
							printf("ReleaseSemaphore error: %d/n", GetLastError());
						}
					
				}

				waveInPrepareHeader(getRecordHandler(),lpHdr, sizeof(WAVEHDR));
				waveInAddBuffer(getRecordHandler(), lpHdr, sizeof(WAVEHDR));
				break;
			default:
				break;
		}
	}
#if RECORD_FILE_ENABLED
    stream->SaveToFile("a.pcm");
    stream->Free();
#endif

    return 0;
}

//??????
DWORD WINAPI voice_udpsend_thread_runner(LPVOID lpParam)
{
	int  result;
	SOCKET      m_Socket;
	unsigned long nAddr;
	struct sockaddr_in To;
	WSADATA     wsaData;
	WORD wVersionRequested;
	SOCKADDR_IN sockaddr;
	int i;
	VadVars *vadstate;
    float indata[FRAME_LEN];
	int vad;
	int nZeroPackageCount;

    wVersionRequested = MAKEWORD(1,1);

    if((result = WSAStartup(wVersionRequested,&wsaData))!=0)
    {
   //      Application->MessageBoxA("Socket Initial Error","Error",MB_OK);
         WSACleanup();
         MessageBox(NULL,"Wrong     WinSock     Version","Error",MB_OK);  
         return -1;
    }

	m_Socket = socket(AF_INET,SOCK_DGRAM,0);
	if(m_Socket == INVALID_SOCKET)
	{
  //      Application->MessageBoxA("Socket Open failed","Error",MB_OK);
        WSACleanup();
        MessageBox(NULL, "Wrong     WinSock     Version", "Error", MB_OK);
        return -1;
    }

#define LocalPort 8302

    memset(&sockaddr,0,sizeof(sockaddr));
    /* ?????     */
    sockaddr.sin_port=htons(LocalPort);
    sockaddr.sin_family=AF_INET;
    sockaddr.sin_addr.S_un.S_addr=htonl(INADDR_ANY);

/*
    int  nZero=0;
    int SndBufLen=1024*64;   //128K
    int RcvBufLen=1024*64;   //128K
    //int  iLen;
    //iLen=sizeof(nZero);           //  SO_SNDBUF
    nZero=SndBufLen;       //128K
    result=setsockopt(m_Socket,SOL_SOCKET,SO_SNDBUF,(char*)&nZero,sizeof((char*)&nZero));
    nZero=RcvBufLen;       //128K
    result=setsockopt(m_Socket,SOL_SOCKET,SO_RCVBUF,(char*)&nZero,sizeof((char*)&nZero));
*/
	nAddr=inet_addr("192.168.2.3");

	To.sin_family=AF_INET;
#define RemotePort 8302
	To.sin_port=htons(RemotePort);
	To.sin_addr.S_un.S_addr=(int)nAddr;
    
	wb_vad_init(&(vadstate));
   
	while(1)
	{
        // Try to enter the semaphore gate.
        DWORD dwWaitResult = WaitForSingleObject(
            ghSemaphore,   // handle to semaphore
            5L);           // zero-second time-out interval


        if(dwWaitResult == WAIT_OBJECT_0)
        {

				//sendto(m_Socket,"dddddddd", 5, 0,(struct sockaddr*)&To,sizeof(struct sockaddr));
				if(pHeaderGet->recordvalid)
				{
						signed short * precdata = (signed short*)(&(pHeaderGet->data[0]));
						int nLength = dwSample/1000*SAMPLINGPERIOD*2*wChannels + sizeof(int) + sizeof(DWORD);

						for(i=0;i<FRAME_LEN;i++)		//??????
						{
								indata[i]= (float)(precdata[i]);
						}
						//vad = wb_vad(vadstate,indata);	//??vad??
						vad =1;//
						if(vad == 1)
						{
							nZeroPackageCount = 0;
						}
						else
						{
							nZeroPackageCount++;
						}

						if((vad==0) && (nZeroPackageCount > 5))
						{
							printf("z=%d\n", nZeroPackageCount);
							//��ǰ����5�������������Թ�
						}
						else
						{
							sendto(m_Socket, &(pHeaderGet->data[0]), nLength, 0,(struct sockaddr*)&To,sizeof(struct sockaddr));
						}
						pHeaderGet->recordvalid = FALSE;
						pHeaderGet = pHeaderGet->pNext;
				}
        }
   }
}

//˫�������ʼ��
void init_audio_buffer()
{
	int i;

	for(i=1;i<BUFCOUNT-1;i++)
	{
		buffers[i].pPrior = &(buffers[i-1]);
		buffers[i].pNext  = &(buffers[i+1]);
        //buffers[i].data = (char*)malloc(dwSample/1000*SAMPLINGPERIOD*2*wChannels);                
		//buffers[i].valid = FALSE;
#if SPEEX_ENABLED
		buffers[i].recordvalid = FALSE;
		buffers[i].speexencodevalid = FALSE;
		buffers[i].speexdecodevalid = FALSE;
#endif
	}
	buffers[0].pPrior =  &(buffers[BUFCOUNT-1]);
	buffers[0].pNext  =  &(buffers[1]);
	//buffers[BUFCOUNT-1].data = (char*)malloc(dwSample/1000*SAMPLINGPERIOD*2*wChannels);
	buffers[BUFCOUNT-1].pPrior =  &(buffers[BUFCOUNT-2]);
	buffers[BUFCOUNT-1].pNext = &(buffers[0]);
	//buffers[BUFCOUNT-1].valid = FALSE;
#if SPEEX_ENABLED
	buffers[BUFCOUNT-1].recordvalid = FALSE;
	buffers[BUFCOUNT-1].speexencodevalid = FALSE;
	buffers[BUFCOUNT-1].speexdecodevalid = FALSE;
#endif
	pHeaderPut = &(buffers[0]);
	pHeaderGet = &(buffers[0]);

#if SPEEX_ENABLED
	pHeaderSpeexEncode = &(buffers[0]);
	pHeaderSpeexDecode = &(buffers[0]);
#endif
}
#endif


typedef int socklen_t;
int InitSocketBuffer(SOCKET fdsocket)
{
    /* 
     * �ȶ�ȡ���������õ���� 
     * ���ԭʼ���ͻ�������С 
     */ 
    int err = -1;        /* ����ֵ */ 
    int snd_size = 0;   /* ���ͻ�������С */ 
    int rcv_size = 0;    /* ���ջ�������С */ 
    socklen_t optlen;    /* ѡ��ֵ���� */ 
    optlen = sizeof(snd_size); 
    err = getsockopt(fdsocket, SOL_SOCKET, SO_SNDBUF,&snd_size, &optlen); 

    if(err<0){ 
        printf("��ȡ���ͻ�������С����\n"); 
    }   
    else
    {
        printf("send buffer size=%d\n", snd_size);
    } 


    err = getsockopt(fdsocket, SOL_SOCKET, SO_RCVBUF,&snd_size, &optlen); 

    if(err<0){ 
        printf("��ȡ���ͻ�������С����\n"); 
    }   
    else
    {
        printf("recv buffer size=%d\n", snd_size);
    } 


    //-------------------------------------------------------------------
    //-------------------------------------------------------------------
    /* 
     * ���ý��ջ�������С 
     */ 
    rcv_size = 1*1024*1024;    /* ���ջ�������СΪ8K */ 
    optlen = sizeof(rcv_size); 
    err = setsockopt(fdsocket,SOL_SOCKET,SO_RCVBUF, (char *)&rcv_size, optlen); 
    if(err<0){ 
        printf("���ý��ջ�������С����\n"); 
    } 
 
    /* 
     * ����������������õ���� 
     * ����޸ĺ��ͻ�������С 
     */ 
    snd_size = 1*1024*1024;    /* ���ջ�������СΪ8K */ 
    optlen = sizeof(snd_size); 
    err = setsockopt(fdsocket, SOL_SOCKET, SO_SNDBUF, (char *)&snd_size, optlen); 
    if(err<0){ 
        printf("��ȡ���ͻ�������С����\n"); 
    }   
 
    //-------------------------------------------------------------------
    //-------------------------------------------------------------------

    err = getsockopt(fdsocket, SOL_SOCKET, SO_SNDBUF,&snd_size, &optlen); 

    if(err<0){ 
        printf("��ȡ���ͻ�������С����\n"); 
    }   
    else
    {
        printf("send buffer size=%d\n", snd_size);
    } 


    err = getsockopt(fdsocket, SOL_SOCKET, SO_RCVBUF,&snd_size, &optlen); 

    if(err<0){ 
        printf("��ȡ���ͻ�������С����\n"); 
    }   
    else
    {
        printf("recv buffer size=%d\n", snd_size);
    } 
}

//���������߳�
DWORD WINAPI voice_udprecv_thread_runner(LPVOID lpParam)
{
	AUDIOBUF tmpbuf;
    int  result;
    SOCKET      m_Socket;
    //unsigned long nAddr;
    struct sockaddr_in serveraddr;
    WSADATA     wsaData;
    WORD wVersionRequested;
    wVersionRequested = MAKEWORD(1,1);

    if((result = WSAStartup(wVersionRequested,&wsaData))!=0)
    {
   //      Application->MessageBoxA("Socket Initial Error","Error",MB_OK);
         WSACleanup();
         MessageBox(NULL,"Wrong     WinSock     Version","Error", MB_OK);  
         return -1;
    }

    m_Socket = socket(AF_INET,SOCK_DGRAM,0);
    if(m_Socket == INVALID_SOCKET)
    {
  //      Application->MessageBoxA("Socket Open failed","Error",MB_OK);
        WSACleanup();
        MessageBox(NULL,"Wrong     WinSock     Version","Error",MB_OK);

        return -1;
    }

	InitSocketBuffer(m_Socket);

    serveraddr.sin_family=AF_INET;
    #define RemotePort 8302
    serveraddr.sin_port=htons(RemotePort);
    serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    if(bind(m_Socket,(struct sockaddr*)&serveraddr,sizeof(serveraddr))<0)
    {
        printf("bind() �˿ڴ���.\r\n");
        //close(sock);
    }

   while(1)
   {
        //sendto(m_Socket,"dddddddd", 5, 0,(struct sockaddr*)&To,sizeof(struct sockaddr));
        //if(pHeaderPut->recvvalid == FALSE)
        {
                signed short * recvdata = (signed short*)(&(pHeaderPut->data[0]));
                int nLength = dwSample/1000*SAMPLINGPERIOD*2*wChannels;
                int dwSenderSize =sizeof(serveraddr);

				
                //if(vad)
                //{
					int recvlength = recvfrom(m_Socket, &(tmpbuf.data[0]), nLength+8, 0, (struct sockaddr*)&serveraddr, &dwSenderSize);
                        //printf("%d\n", recvlength);
               // }
				if((pHeaderGet->frameNO > 0) && (tmpbuf.frameNO < pHeaderGet->frameNO))
				{
					printf("������̫������ݰ�\n");
				}
				else
				{
					memcpy(&(pHeaderPut->data[0]),  &(tmpbuf.data[0]), recvlength);
					printf("rNO=%d\n", pHeaderPut->frameNO);
					fprintf(logFile, "rNO=%d\n", pHeaderPut->frameNO);
					pHeaderPut->recvvalid = TRUE;
					pHeaderPut = pHeaderPut->pNext;
				}
                if (!ReleaseSemaphore( 
                    ghSemaphore,  // handle to semaphore - hSemaphore��Ҫ���ӵ��ź������
                    1,            // increase count by one - lReleaseCount�����ӵļ���
                    NULL) )       // not interested in previous count - lpPreviousCount������ǰ����ֵ����
                {
                    printf("ReleaseSemaphore error: %d/n", GetLastError());
                }
        }
   }
}

static int nZeroPackageCount = 0;//����VAD=0����

//���������߳�
DWORD WINAPI voice_play_thread_runner(LPVOID   lpParam)   
{
	VadVars *vadstate;
    float indata[FRAME_LEN];
	int vad;
	int i;

	HANDLE eventPlay = (HANDLE)(lpParam);

	if( 0 != startPlaying(GetCurrentThreadId() )  )
	{
		printf("Start Playing Failed!\n");
		return -1;
	}

    wb_vad_init(&(vadstate));

	while(1)   
	{

        // Try to enter the semaphore gate.
        DWORD dwWaitResult = WaitForSingleObject(
            ghSemaphore,   // handle to semaphore
            INFINITE);           // zero-second time-out interval


        if(dwWaitResult == WAIT_OBJECT_0)
        {

		if(pHeaderGet->recvvalid == TRUE)
		{

			signed short * precdata = (signed short*)(&(pHeaderGet->data[0]));
			int nLength = dwSample/1000*SAMPLINGPERIOD*2*wChannels;

			for(i=0;i<FRAME_LEN;i++)		//??????
			{
					indata[i]= (float)(precdata[i]);
			}
			vad = wb_vad(vadstate,indata);	//??vad??

			if(vad == 1)
			{
				nZeroPackageCount = 0;
			}
			else
			{
				nZeroPackageCount++;
			}

			printf("             pNO=%d\n", pHeaderGet->frameNO);
			fprintf(logFile, "             pNO=%d\n", pHeaderGet->frameNO);
#if 1
			if((vad==0) && (nZeroPackageCount > 30))
			{
				printf("z=%d\n", nZeroPackageCount);
				nZeroPackageCount = 0;
#if 1
				while(pHeaderGet != pHeaderPut)
				{
					pHeaderGet->recvvalid = FALSE;//��Ϊ�����������ݰ�
					pHeaderGet = pHeaderGet->pNext;
				}
#endif
				//��ǰ����5�������������Թ�
			}
			else
#endif
			{
				if(pHeaderGet->pPrior->frameNO > pHeaderGet->frameNO)
				{
					printf("���ܲ��űȵ�ǰ��������ݰ�, pHeaderGet->pPrior->frameNO=%d,pHeaderGet->frameNO=%d\n",pHeaderGet->pPrior->frameNO, pHeaderGet->frameNO);
					fprintf(logFile, "���ܲ��űȵ�ǰ��������ݰ�, pHeaderGet->pPrior->frameNO=%d,pHeaderGet->frameNO=%d\n",pHeaderGet->pPrior->frameNO, pHeaderGet->frameNO);
				}

				if( 0 != playWavData((char*)&(pHeaderGet->data[0]), dwSample/1000*SAMPLINGPERIOD*2*wChannels))
				{
					printf("Playing Wave Data Failed!\n");
				}
			}
			pHeaderGet->recvvalid = FALSE;

			if(pHeaderGet->frameNO - pHeaderGet->pPrior->frameNO!= 1)
			{
				printf("������,pPrior->frameNO=%d, pHeaderGet->frameNO=%d\n",pHeaderGet->pPrior->frameNO, pHeaderGet->frameNO);
				fprintf(logFile, "������,pPrior->frameNO=%d, pHeaderGet->frameNO=%d\n",pHeaderGet->pPrior->frameNO, pHeaderGet->frameNO);
			}

			pHeaderGet = pHeaderGet->pNext;	
		}
        }

	}   
	waveOutReset(g_playHandler);
	waveOutClose(g_playHandler);
	return   0;   
}

void main()
{
	logFile = _wfopen(L"log.txt", L"w");

    eventRecord = CreateEvent(NULL,   TRUE,   FALSE,   NULL); 	
    eventPlay   = CreateEvent(NULL,   TRUE,   FALSE,   NULL);

    dwSample = 16000;
    wChannels = 1;

    // Create a semaphore with initial and max counts of MAX_SEM_COUNT
    ghSemaphore = CreateSemaphore( 
        NULL,           // default security attributes - lpSemaphoreAttributes���ź����İ�ȫ����
        MAX_SEM_COUNT,  // initial count - lInitialCount�ǳ�ʼ�����ź���
        MAX_SEM_COUNT,  // maximum count - lMaximumCount�������ź������ӵ����ֵ
        NULL);          // unnamed semaphore - lpName���ź���������

    init_audio_buffer();

#if 0
    hRecord = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0,
          (LPTHREAD_START_ROUTINE)voice_record_thread_runner,
          (LPVOID)eventRecord,0, &threadRecord);

    hUDPSend = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0,
          (LPTHREAD_START_ROUTINE)voice_udpsend_thread_runner,
          (LPVOID)eventUDPSend,0, &threadUDPSend); 
#else
//    eventRecord = CreateEvent(NULL,   TRUE,   FALSE,   NULL);
//    eventPlay   = CreateEvent(NULL,   TRUE,   FALSE,   NULL);

    hPlay = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0,
          (LPTHREAD_START_ROUTINE)voice_play_thread_runner,
          (LPVOID)eventPlay, 0, &threadPlay);

    hUDPRecv = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0,
          (LPTHREAD_START_ROUTINE)voice_udprecv_thread_runner,
          (LPVOID)eventUDPRecv,0, &threadUDPRecv);
#endif
	while(1)
	{
		Sleep(10000); 
	}
}

#if 0
void main1()
{	
		int i,frame=0,temp,vad; 
		float indata[FRAME_LEN];
		VadVars *vadstate;					
		FILE *fp1;
		fp1=fopen("inls1.wav","rb");
		wb_vad_init(&(vadstate));			//vad��ʼ��
		while(!feof(fp1))
		{	
			frame++;
			for(i=0;i<FRAME_LEN;i++)		//��ȡ�����ļ�
			{	
				indata[i]=0;
				temp=0;
				fread(&temp,2,1,fp1);
				indata[i]=(float)temp;
				if(indata[i]>65535/2)
				indata[i]=indata[i]-65536;
			}
			vad=wb_vad(vadstate,indata);	//����vad���
			printf("%d \n",vad);
		}
		printf("ok!");
		fcloseall();
		getchar();
}
#endif
