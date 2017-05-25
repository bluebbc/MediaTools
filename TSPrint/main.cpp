
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
};

#include <vector>
#include <string>
#include <iostream>

using namespace std;

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h> 
#endif

struct Config{
	Config():_bDelJpg(false),_num(0){}
	~Config(){}
	void Init(int alltime, int framerate)
	{
		int interval = alltime / (_num+1);
		if(interval<=0)
			interval=1;

		for(int i=1;i<=_num;i++){
			_vecTime.push_back(i*interval);
		}
	}

	void MakeFileName()
	{
		 std::string::size_type found = _inputFile.find_last_of("/\\");
		 _inputFileName = _inputFile.substr(found+1);
	}

	int _num;
	std::vector<int> _vecTime;

	std::string _inputFile;
	std::string _inputFileName;
	std::string _outputDir;
	bool _bDelJpg;
};

typedef struct pts_info_s{
	bool flag;
	int64_t pts;
}pts_info_s;

void Usage()
{
	printf("\tpts : bPrtV bPrtA filename\n\n");
}

int main(int argc, char* argv[])
{
	Config cfg;

	Usage();
	if (argc != 4)
		return 0;

	int bPrintVideo = 1, bPrintAudio = 1;
	bPrintVideo = atoi(argv[1]);
	bPrintAudio = atoi(argv[2]);
	cfg._inputFile = argv[3];

	AVFormatContext	*pFormatCtx;
	int				i, videoindex, audioindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if(avformat_open_input(&pFormatCtx,cfg._inputFile.c_str(),NULL,NULL)!=0){
		printf("Couldn't open input stream.（无法打开输入流）\n");
		return -1;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Couldn't find stream information.（无法获取流信息）\n");
		return -1;
	}
	videoindex=-1;
	audioindex = -1;
	for(i=0; i<pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
		}
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioindex = i;
		}
	}

	if (videoindex == -1 && audioindex == -1)
	{
		printf("error\n");
		return -1;
	}

	AVFrame	*pFrame;
	pFrame=av_frame_alloc();

	AVPacket pkt;
	AVPacket *packet = &pkt;
	

	//输出一下信息-----------------------------
	av_dump_format(pFormatCtx,0,cfg._inputFile.c_str(),0);

	int nVFrameCount = 0, nAudioFrameCount = 0;
	while(av_read_frame(pFormatCtx, packet)>=0)
	{
		if(packet->stream_index==0)
		{
			if (bPrintVideo != 0){
				printf("video	%d	--	%12I64d	%12I64d	(size:%d)\n", 
					nVFrameCount, packet->pts, packet->dts, packet->size);
			}
			nVFrameCount++;
		}else{
			if (bPrintAudio != 0)
			{
				printf("audio	%d	--	%12I64d	%12I64d	(size:%d)\n",
					nAudioFrameCount, packet->pts, packet->dts, packet->size);
			}
			nAudioFrameCount++;
		}
		av_free_packet(packet);
	}

	avformat_close_input(&pFormatCtx);

	return 0;
}
