/********************************************************************
*																	*
* Author: keith70													*
* Date modified: 2012.04.06											*
* Description: Cut image(generated by "ImageReader") into blocks	*
*																	*
********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "func.h"

#ifndef MAX_PATH
#define MAX_PATH 255
#endif

#define PART_WIDTH  800  //分块宽度
#define PART_HEIGHT 800  //分块高度
#define PART_EXC    40   //冗余度(像素)

char* GetDir(char* dir, char* path); //从文件路径获取所在目录
void InitArg(char* path, char* dir); //请求并读入参数
int CreateMultDir(const char* sPath); //创建多级目录
int SaveBitsPart(char* fileName, unsigned char* srcBits,  //要保存的文件名,源字节流
				unsigned srcWidth, unsigned srcHeight, //总体尺寸
				unsigned width, unsigned height, //分块尺寸
				unsigned x, unsigned y, //起点坐标(坐上角为原点,x->width,y->height),注意这里只是索引
				int excPixel, int ID);  //冗余度(像素), 块序号


int main(int argc, char *argv[])
{
	setvbuf(stdout, (char *)NULL, _IONBF, 0); //禁用缓冲

	char imgReadPath[MAX_PATH] = {'\0'}; //main参数1: 输入文件路径
	char saveDir[MAX_PATH] = {'\0'};     //main参数2: 输出目录
	FILE* readFile;   //读入文件指针
	FILE* outCfgFile;   //输出配置文件指针
	unsigned width,height; //读入图像的宽高
	unsigned char* bits;

	//获取当前工作目录
	if(NULL == getcwd(currentDir, sizeof(currentDir)))
	{
		fprintf(stderr, "Get current working directory failed!");
		return 0;
	}
	strcat(currentDir, "\\");

	switch(argc)
	{
	case 1: //直接运行
		InitArg(imgReadPath, saveDir);
		break;
	case 2: //命令行调用,一个参数,保存路径默认为输入文件所在目录
		strcpy(imgReadPath, argv[1]); 
		GetDir(saveDir, imgReadPath);
		break;
	case 3: //命令行调用,两个参数
		strcpy(imgReadPath, argv[1]);
		strcpy(saveDir, argv[2]);
		break;
	default: //错误
		fprintf(stderr, "Arguments Error!");
		return 0;
	}
	
	//检查输入文件是否存在
	if(NULL == strchr(imgReadPath, ':'))
	{
		char tmp[MAX_PATH] = {'\0'};
		strcpy(tmp, currentDir);
		strcat(tmp, imgReadPath);
		strcpy(imgReadPath, tmp);
	}
	readFile = fopen(imgReadPath, "rb"); 
	if(NULL == readFile)
	{
		fprintf(stderr, "Input file path not correct!");
		return 0;
	}

	//检查或创建输出目录
	if(saveDir[strlen(saveDir)-1] != '\\') 
	{
		strcat(saveDir, "\\");  //格式化
	}
	if(0 == CreateMultDir(saveDir))
	{
		fprintf(stderr, "Create save file directory failed!");
		return 0;
	}

	//读入文件数据
	char mark[5] = {'\0'};
	fread(mark, sizeof(char), 4, readFile);
	if(strcmp(mark, "ImgB") != 0)
	{
		fprintf(stderr, "Input file format not correct!");
		return 0;
	}
	fread(&width, sizeof(unsigned), 1, readFile);
	fread(&height, sizeof(unsigned), 1, readFile);
	unsigned bitsCount = width*height;
	bits = (unsigned char*)malloc(bitsCount*sizeof(unsigned char)); //宽x高
	fread(bits, sizeof(unsigned char), bitsCount, readFile);
	fclose(readFile);

	//初始化配置文件
	char cfgPath[MAX_PATH];
	strcpy(cfgPath, saveDir);
	strcat(cfgPath, "PartConfig.conf");
	outCfgFile = fopen(cfgPath, "wt+");
	if(NULL == outCfgFile)
	{
		fprintf(stderr, "Create config file failed!");
		return 0;
	}
	char strSizeTmp[5][10];
	itoa(width, strSizeTmp[0], 10);
	itoa(height, strSizeTmp[1], 10);
	itoa(PART_WIDTH, strSizeTmp[2], 10);
	itoa(PART_HEIGHT, strSizeTmp[3], 10);
	itoa(PART_EXC, strSizeTmp[4], 10);

	fprintf(outCfgFile, "Input_Image_data_file_path=%s\n", imgReadPath);
	fprintf(outCfgFile, "Output_parts_dir=%s\n", saveDir);
	fprintf(outCfgFile, "Input_size:width=%s\n", strSizeTmp[0]);
	fprintf(outCfgFile, "Input_size:height=%s\n", strSizeTmp[1]);
	fprintf(outCfgFile, "Output_size_standard:width=%s\n", strSizeTmp[2]);
	fprintf(outCfgFile, "Output_size_standard:height=%s\n", strSizeTmp[3]);
	fprintf(outCfgFile, "OutPut_size_excess_in_pixel=%s\n", strSizeTmp[4]);

	//处理数据
	int ID = 0;
	char strTmp[10];
	char fileName[MAX_PATH]; //总文件名
	int widthNum = (width + PART_WIDTH/2) / PART_WIDTH;
	int heightNum = (height + PART_HEIGHT/2) / PART_HEIGHT;
	fprintf(outCfgFile, "Part_count=%d\n", (heightNum * widthNum));
	fprintf(outCfgFile, "Part_begin\n");
	for(int y=0; y < heightNum; y++)
	{
		//输出进度
		//fflush(stdout);
		fprintf(stdout, "%d%%\n", (int)(y/(heightNum-1.0)*100));

		for(int x=0; x < widthNum; x++)
		{
			itoa(ID, strTmp, 10); //int -> char[]
			sprintf(fileName, "%sPart_%s.dat", saveDir, strTmp); //  格式化文件名
			int status = SaveBitsPart(fileName, bits, width, height, 
				PART_WIDTH, PART_HEIGHT, x, y, PART_EXC, ID);
			if(0 == status)
			{
				fprintf(stderr, "Save data to file failed!");
				return 0;
			}
			fprintf(outCfgFile, "Part_%d:path=%s\n", ID, fileName);
			ID++;
		}
	}
	fprintf(outCfgFile, "Part_end\n");

	//完成
	//fflush(stdout);
	fprintf(stdout, "100%% Done!\n");
	fclose(outCfgFile);
	return 0;
}
