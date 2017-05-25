
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

int nal_detail(AVPacket *org);

int main(int argc, char* argv[])
{

	string _inputFile = argv[1];

	AVFormatContext	*pFormatCtx;
	int				i, videoindex, audioindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, _inputFile.c_str(), NULL, NULL) != 0){
		printf("Couldn't open input stream.（无法打开输入流）\n");
		return -1;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Couldn't find stream information.（无法获取流信息）\n");
		return -1;
	}
	videoindex = -1;
	audioindex = -1;
	for (i = 0; i<pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
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
	pFrame = av_frame_alloc();

	AVPacket pkt;
	AVPacket *packet = &pkt;


	//输出一下信息-----------------------------
	av_dump_format(pFormatCtx, 0, _inputFile.c_str(), 0);

	while (av_read_frame(pFormatCtx, packet) >= 0)
	{
		if (packet->stream_index == videoindex)
		{
			nal_detail(packet);
		}
		av_free_packet(packet);
	}

	avformat_close_input(&pFormatCtx);

	return 0;
}

enum state_e{
	S_start = 0,
	S_0_1,
	S_0_2,
	S_0_3,
	S_end_2_0,
	S_end_3_0
};

typedef struct nal_s{
	unsigned char type;
	int startCodeLen;
	int posHead;
	int size;
	std::vector<nal_s> vecSons;
}nalu_s, pktu_s;

int nal_parser_sub(unsigned char *pNal, nal_s &node)
{
	std::vector<nal_s> &vSons = node.vecSons;
	int size = node.size;

	// 找到所有的0x00000001 或者 0x000001
	int state = S_start;
	for (int i = 0; i < size; i++){
		switch (state)
		{
		case S_start:
			if (pNal[i] == 0)
				state = S_0_1;
			break;
		case S_0_1:
			if (pNal[i] == 0)
				state = S_0_2;
			else
				state = S_start;
			break;
		case S_0_2:
			if (pNal[i] == 0)
				state = S_0_3;
			else if (pNal[i] == 1)
			{
				nal_s ns = { 0 };
				ns.posHead = i - 2;
				ns.type = pNal[i + 1] & 0x1f;
				ns.startCodeLen = 3;
				vSons.push_back(ns);
				state = S_start;
			}
			else
				state = S_start;
			break;
		case S_0_3:
			if (pNal[i] == 1)
			{
				nal_s ns = { 0 };
				ns.posHead = i - 3;
				ns.type = pNal[i + 1] & 0x1f;
				ns.startCodeLen = 4;
				vSons.push_back(ns);
			}
			state = S_start;
			break;
		default:
			break;
		}
	}

	if (vSons.size() == 0)
		return -1;

	// 生成size信息
	int i=0;
	for (i = 0; i < vSons.size() - 1; i++){
		printf("i:%d max:%d\n", i, vSons.size() - 1);
		vSons[i].size = vSons[i + 1].posHead - vSons[i].posHead - vSons[i].startCodeLen;
	}
	vSons[i].size = size - vSons[i].posHead - vSons[i].startCodeLen;

	// 生成绝对位置
	for (i = 0; i < vSons.size(); i++){
		vSons[i].posHead += node.posHead;
	}

	for (i = 0; i < vSons.size() - 1; i++){
		if (vSons[i].size == 0){
			vSons[i].type = 0;
		}
	}
	return 0;
}

int nal_detail(AVPacket *org)
{
	unsigned char *pNal = org->data;
	nal_s nalInfo = { 0 };
	nalInfo.posHead = 0;
	nalInfo.type = -1;
	nalInfo.size = org->size;

	nal_parser_sub(&pNal[0], nalInfo);

	printf("nalType");
	for (int i = 0; i < nalInfo.vecSons.size(); i++)
	{
		printf("%2d ", nalInfo.vecSons[i].type);
	}
	printf("\n");
	return 0;
}
