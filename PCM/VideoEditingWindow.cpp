#include "paint_canvas.h"
#include "cameraViewer.h"
#include "VideoEditingWindow.h"
#include "MyBayesian.h"
#include "frameDif.h"
#include "GlobalObject.h"
#include <QFileDialog>
#include <QMessageBox>
#include "readFrame.h"
#include "camerawidget.h"
using namespace cv;
VideoEditingWindow* VideoEditingWindow:: window_ = 0;

void mat2QImage(const Mat& srcMat, QImage& desQImage)
{
	int nChannel = srcMat.channels();
	if (nChannel == 3)
	{
		Mat srcTemp = Mat(srcMat.rows, srcMat.cols, srcMat.type());
		cvtColor(srcMat, srcTemp, CV_BGR2RGB);
		//unsigned char * matBits=(unsigned char *)srcMat.data;
		//unsigned char * qimageBits;
		desQImage = QImage(srcTemp.cols, srcTemp.rows, QImage::Format_RGB888);
		//int w=desQImage.width(),h=desQImage.height(),scanLine=desQImage.bytesPerLine();
		memcpy(desQImage.bits(), srcTemp.data, srcTemp.cols*srcTemp.rows * 3 * sizeof(unsigned char));	//ֱ�Ӹ������ݣ����ڿ������cols����4�ı������������ڴ��������⣬�Ǿ�һ��һ�еĽ���
																										//desQImage = QImage((const unsigned char*)(srcTemp.data),srcTemp.cols,srcTemp.rows,srcTemp.cols*srcTemp.channels(),QImage::Format_RGB888);		//desQImage��srcTemp�����ڴ棬srcTemp����ʱ��desQImage������Ҳû��
																										//qimageBits=(unsigned char *)desQImage.bits();
	}
	else if (nChannel == 4 || nChannel == 1)
	{
		desQImage = QImage((const unsigned char*)srcMat.data, srcMat.cols, srcMat.rows, srcMat.step, QImage::Format_ARGB32);
	}
}

void QImage2Mat(const QImage& srcQImage, Mat& desMat)
{
	cv::Mat matQ = cv::Mat(srcQImage.height(), srcQImage.width(), CV_8UC4, (uchar*)srcQImage.bits(), srcQImage.bytesPerLine());
	desMat = Mat(matQ.rows, matQ.cols, CV_8UC3);
	int from_to[] = { 0,0, 1,1, 2,2 };
	cv::mixChannels(&matQ, 1, &desMat, 1, from_to, 3);
}

inline Mat mattingMethod(const Mat& trimapMat, Mat& srcMat, Mat& alphaMat , Mat& back_ground)
{

	/*float Time=0;
	string TimeFile ="SSEMattingTime.txt";
	ofstream ftimeout(TimeFile,ios::app);
	double TCount = (double)cvGetTickCount();*/

	Mat compositeResult;
	//BayesianMatting matting(srcMat,trimapMat);
	MyBayesian matting(srcMat, trimapMat, DEFAULT_LAMDA, DEFAULT_ITERATION_TIMES);
	matting.Solve();

	/*TCount = (double)cvGetTickCount()-TCount;
	Time = TCount/(cvGetTickFrequency()*1000000);
	ftimeout<<Time<<"s"<<endl;

	float Time2=0;
	string TimeFile2 ="SSECompositeTime.txt";
	ofstream ftimeout2(TimeFile2,ios::app);
	double TCount2 = (double)cvGetTickCount();*/

	//matting.Composite(back_ground, &compositeResult);	
	matting.Composite_SSE(back_ground, &compositeResult);

	/*TCount2 = (double)cvGetTickCount()-TCount2;
	Time2 = TCount2/(cvGetTickFrequency()*1000000);
	ftimeout2<<Time2<<"s"<<endl;
	*/
	compositeResult.convertTo(compositeResult, CV_8UC3, 255.0);
	matting.getAlphaResult(alphaMat);
	alphaMat.convertTo(alphaMat, CV_8UC3, 255.0);
	return compositeResult;
}



VideoEditingWindow::VideoEditingWindow(QWidget *parent /*= 0*/):
	preprocessThread(&preprocessor), cameraWidget(NULL), rfWidget(NULL), currentFrame(NULL),
	timer(NULL)
{
	ui_.setupUi(this);
	ui_.videoFrame->setMouseTracking(true); 
	//ui_.SourceVideo->resize(QSize(ui_.SourceVideo->width(),
	//	ui_.SourceVideo->height() + 1000));
		
	setUp();
}

VideoEditingWindow::~VideoEditingWindow()
{
	window_ = NULL;
	if (capture.isOpened())
		capture.release();
	/*if(curImage)
	delete curImage;  */
	if (timer)
		delete timer;
	if (cameraWidget)
		delete  cameraWidget;
	if (rfWidget)
		delete rfWidget;
}


void VideoEditingWindow::setUp()
{
	setUpSourceVideo();
	setUpToolbox();
}
void VideoEditingWindow::setUpSourceVideo()
{

	opengllayout = new QVBoxLayout(ui_.frame_manipulate);
//	PaintCanvas* canvas = new PaintCanvas(QGLFormat::defaultFormat(), 0, ui_.frame_manipulate, this);
	camera_viewer = new CameraViewer(Global_Canvas->camera());
	QObject::connect((const QObject*)Global_Canvas->camera()->frame(), SIGNAL(manipulated()), camera_viewer, SLOT(updateGL()));
	QObject::connect((const QObject*)Global_Canvas->camera()->frame(), SIGNAL(spun()), camera_viewer, SLOT(updateGL()));
	// Also update on camera change (type or mode)
	QObject::connect(Global_Canvas, SIGNAL(cameraChanged()), camera_viewer, SLOT(updateGL()));
	camera_viewer->setWindowTitle("Camera viewer: " + QString::number(0));		
	opengllayout->addWidget(camera_viewer);
	camera_viewer->updateGL();
  
}
void VideoEditingWindow::setUpToolbox()
{
	ui_.kernelSizeSlider->setValue(INIT_KERNELSIZE * 10);
	ui_.brushSizeSlider->setValue(15);
	ui_.actionClose->setEnabled(false);
	ui_.actionSave_as->setEnabled(false);
	ui_.pushButton_pause->setEnabled(false);
	ui_.pushButton_play->setEnabled(false);
	ui_.pushButton_previousframe->setEnabled(false);
	ui_.pushButton_nextframe->setEnabled(false);
	ui_.NextInitKey->setEnabled(false);
	//ui_.ResPause->setEnabled(false);
	//ui_.ResPlay->setEnabled(false);
	//ui_.ResPreFrame->setEnabled(false);
	//ui_.ResNextFrame->setEnabled(false);
	ui_.toolsforstep1->setEnabled(false);
	ui_.SetKey->setEnabled(false);
	ui_.SelectArea->setChecked(true);
	ui_.BrushBack->setChecked(false);
	ui_.BrushCompute->setChecked(false);
	ui_.BrushFore->setChecked(false);
	ui_.ForeCut->setChecked(false);
	ui_.BackCut->setChecked(false);
	ui_.Init_Key_Frame_by_Diff->setEnabled(false);
	ui_.TrimapInterpolation->setEnabled(false);
	ui_.MattingVideo->setEnabled(false);
	ui_.ChangeBackground->setEnabled(false);

	timer = new QTimer(this);

	connect(timer, SIGNAL(timeout()), this, SLOT(nextFrame()));
	connect(ui_.actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
	connect(ui_.actionSave_as, SIGNAL(triggered()), this, SLOT(saveas()));
	connect(ui_.actionClose, SIGNAL(triggered()), this, SLOT(closeFile()));
	connect(ui_.actionCamera, SIGNAL(triggered()), this, SLOT(camera()));
	connect(ui_.actionRead_frame, SIGNAL(triggered()), this, SLOT(read_frame()));
	connect(ui_.actionWrite_video, SIGNAL(triggered()), this, SLOT(write_video()));
	connect(ui_.actionMatting, SIGNAL(triggered()), this, SLOT(matting()));
	connect(ui_.ChangeBackground, SIGNAL(clicked()), this, SLOT(changeBackground()));
	connect(ui_.actionAlpha2trimap, SIGNAL(triggered()), this, SLOT(alpha2Trimap()));
	connect(ui_.actionSplit_Video, SIGNAL(triggered()), this, SLOT(splitVideo()));
	connect(ui_.actionCompute_gradient, SIGNAL(triggered()), this, SLOT(computeGradient()));

	connect(ui_.pushButton_pause, SIGNAL(clicked()), this, SLOT(pause()));
	connect(ui_.pushButton_play, SIGNAL(clicked()), this, SLOT(play()));
	connect(ui_.pushButton_nextframe, SIGNAL(clicked()), this, SLOT(nextFrame()));
	connect(ui_.pushButton_previousframe, SIGNAL(clicked()), this, SLOT(preFrame()));
	connect(ui_.spinBox_turntoframe, SIGNAL(valueChanged(int)), this, SLOT(turnToFrame(int)));
	connect(ui_.SetKey, SIGNAL(clicked()), this, SLOT(setKeyFrame()));
	connect(ui_.NextInitKey, SIGNAL(clicked()), this, SLOT(nextInitKey()));

	connect(ui_.SelectArea, SIGNAL(clicked()), this->ui_.videoFrame, SLOT(setSelectTool()));
	connect(ui_.BrushBack, SIGNAL(clicked()), this->ui_.videoFrame, SLOT(setBackgroundBrush()));
	connect(ui_.BrushCompute, SIGNAL(clicked()), this->ui_.videoFrame, SLOT(setComputeAreaBrush()));
	connect(ui_.BrushFore, SIGNAL(clicked()), this->ui_.videoFrame, SLOT(setForegroundBrush()));
	connect(ui_.ForeCut, SIGNAL(clicked()), this->ui_.videoFrame, SLOT(setForegroundCut()));
	connect(ui_.BackCut, SIGNAL(clicked()), this->ui_.videoFrame, SLOT(setBackgroundCut()));
	connect(ui_.Grabcut, SIGNAL(clicked()), this, SLOT(grabcutIteration()));
	connect(ui_.ShowTrimap, SIGNAL(clicked()), this, SLOT(showTrimap()));
	connect(ui_.showKeyFrameNo, SIGNAL(clicked()), this, SLOT(showKeyFrameNO()));
	connect(ui_.MattingSingleFrame, SIGNAL(clicked()), this, SLOT(mattingFrame()));

	connect(ui_.Init_Key_Frame_by_Diff, SIGNAL(clicked()), this, SLOT(initKeyframe()));
	connect(ui_.TrimapInterpolation, SIGNAL(clicked()), this, SLOT(trimapInterpolation()));
	connect(ui_.MattingVideo, SIGNAL(clicked()), this, SLOT(mattingVideo()));
	connect(ui_.ChangeBackground, SIGNAL(clicked()), this, SLOT(changeBackground()));

	connect(ui_.kernelSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(setGaussianKernelSize(int)));
	connect(ui_.brushSizeSlider, SIGNAL(valueChanged(int)), ui_.videoFrame, SLOT(setBrushSize(int)));
	/*connect(this->SrcVideo,SIGNAL(changeBrushByKey(SrcWidget::BrushMode)),
	this, SLOT(changeBrushByKey(SrcWidget::BrushMode)));*/
	connect(ui_.videoFrame, SIGNAL(selectionTool()), ui_.SelectArea, SLOT(click()));
	connect(ui_.videoFrame, SIGNAL(increaseWidth(bool)), this, SLOT(increaseWidth(bool)));// ����trimap����
	connect(&preprocessThread, SIGNAL(changeTrimap()), this, SLOT(refresh()));
	connect(ui_.videoFrame, SIGNAL(changeMask()), this, SLOT(cutInteract()));
	connect(ui_.videoFrame, SIGNAL(changeRect()), this, SLOT(cutSelect()));

	back_ground = imread("./background.jpg");	//Ĭ�ϱ���

	preprocessThread.start();

}

void VideoEditingWindow::openFile()
{
	QString cur_dir = "./";
	QString fileName = QFileDialog::getOpenFileName(this, "Open File", cur_dir, "Vedio (*.avi)");
	if (fileName.isEmpty())
		return;
	std::string  str =  fileName.toLocal8Bit().constData();

	const char* ch = str.c_str();

	QString current_file_path;
	QFileInfo fi;
	fi = QFileInfo(fileName);
	current_file_path = fi.absolutePath();
	current_file_path += "/";
	currentfilePath = current_file_path.toStdString();
	QDir dir(current_file_path);
	dir.mkdir("Trimap");
	dir.mkdir("Result");
	dir.mkdir("Alpha");
	if (ch)
	{
		capture.open(ch);
		if (capture.isOpened())
		{
			//��ȡ֡��
			rate = capture.get(CV_CAP_PROP_FPS);
			//��֡��ļ��ʱ��:
			delay = 1000 / rate;
			//��ȡ��֡��
			totalFrameNumber = capture.get(CV_CAP_PROP_FRAME_COUNT);
			if (totalFrameNumber <= 0)		//��Щ��Ƶ�����Ϊ0��������ݲ����ǣ����趼�ɳɹ���ȡ
			{
				QMessageBox::information(this, "Information", "Total frame number is zero!", QMessageBox::Ok);
			}
			else
			{
				if (!frame.empty())
				{
					frame.clear();
				}
				if (!keyFrameNo.empty())
				{
					keyFrameNo.clear();
				}
				frame.resize(totalFrameNumber);
				for (long i = 0; i < totalFrameNumber; i++)
				{
					capture >> curframe;
					if (!curframe.empty())
					{
						frame.at(i).framePos = i;
						frame.at(i).trimap = Mat(curframe.rows, curframe.cols, CV_8UC1, cv::Scalar(0));	//Ĭ��Ϊ����������Ϊ���������Ҳ��������Ŀ����Ը��󣿣���						
					}
				}
			}

			currentframePos = 0;
			currentFrame = &frame.at(currentframePos);		//ָ��ǰ����Ƶ֡
			if (currentFrame->ifKeyFrame == false)
			{
				ui_.SetKey->setChecked(false);
			}
			else if (currentFrame->ifKeyFrame == true)
			{
				ui_.SetKey->setChecked(true);
			}

			capture.set(CV_CAP_PROP_POS_FRAMES, currentframePos);	//��ͷ��ʼ��ȡԭʼ֡
			capture >> curframe;

			if (!curframe.empty())
			{
				cv::resize(back_ground, back_ground, cv::Size(curframe.cols, curframe.rows), 0, 0, CV_INTER_LINEAR);	//�����±�����С
				preprocessThread.setImage(curframe);
				cvtColor(curframe, tempFrame, CV_BGR2RGB);
				curImage = QImage((const unsigned char*)(tempFrame.data), tempFrame.cols, tempFrame.rows, QImage::Format_RGB888);
				ui_.videoFrame->clearRegion();
				ui_.videoFrame->setImage(&curImage);
				ui_.videoFrame->updateDisplayImage();
				

			}
			else
			{
				QMessageBox::information(this, "Information", "Current Frame is empty!", QMessageBox::Ok);
			}
		}

	}
	else
		return;
	if (timer->isActive())
	{
		timer->stop();
	}
	ui_.pushButton_play->setEnabled(true);
	ui_.pushButton_previousframe->setEnabled(true);
	ui_.pushButton_nextframe->setEnabled(true);
	ui_.pushButton_pause->setEnabled(false);
	ui_.SetKey->setEnabled(false);
	ui_.toolsforstep1->setEnabled(true);
	ui_.cur_frame_idx->setNum(1);
	ui_.total_framenum->setNum(totalFrameNumber);
//	ui_.ResTotalNum->setNum(totalFrameNumber);
	ui_.spinBox_turntoframe->setMaximum(totalFrameNumber);
	ui_.spinBox_turntoframe->setValue(1);
	ui_.SelectArea->click();
	ui_.Init_Key_Frame_by_Diff->setEnabled(true);
	ui_.TrimapInterpolation->setEnabled(true);
	ui_.MattingVideo->setEnabled(true);
	ui_.ChangeBackground->setEnabled(true);
}

void VideoEditingWindow::closeFile()
{

}

void VideoEditingWindow::saveas()
{
	QString cur_dir = "G:\\Liya\\Task\\video matting\\Data";
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), cur_dir, "Vedio (*.avi)");

	std::string str = fileName.toStdString();
	const char* ch = str.c_str();
	if (ch)
	{
	}
}
/*
   read image sequence and show in a single window
*/
void VideoEditingWindow::read_frame()
{
	QString cur_dir = "./";
	QString filepath = QFileDialog::getExistingDirectory(this, "Get existing directory", cur_dir);

	if (!filepath.isEmpty())
	{
		filepath += "\\";
		rfWidget = new ReadFrameWidget(filepath);
		rfWidget->setWindowTitle("Read Frame");
		rfWidget->setVisible(true);
		rfWidget->show();
	}
}

void VideoEditingWindow::camera()
{
	cameraWidget = new CameraWidget();
	cameraWidget->setWindowTitle("CameraVideo");
	cameraWidget->setVisible(true);
	cameraWidget->show();
}
/*
  matting images in opendir 
  opendir should include input dir (source,trimap)
*/
void VideoEditingWindow::matting()
{
	/*float totalTime = 0;
	string TimeFile = "oriTotalTime.txt";
	ofstream ftime(TimeFile,ios::app);*/

	QString cur_dir = "./";
	QString filepath = QFileDialog::getExistingDirectory(this, "Get existing directory", cur_dir);

	Mat src, trimap, tmap;

	if (!filepath.isEmpty())
	{
		QMessageBox::information(this, "Information", "Bat Matting begin!", QMessageBox::Ok);

		double TCount = (double)cvGetTickCount();

		QString srcFilepath = filepath + "\\source\\";
		QString trimapFilepath = filepath + "\\trimap\\";
		QString resultFilepath = filepath + "\\result\\";
		QString alphaFilepath = filepath + "\\alpha\\";

		QDir dir(filepath);
		dir.mkdir("result");
		dir.mkdir("alpha");

		QFileInfoList srcList;
		QFileInfo srcFileInfo;
		QDir srcDir(srcFilepath);
		srcDir.setFilter(QDir::Files | QDir::NoSymLinks);
		srcDir.setSorting(QDir::Name);
		srcList = srcDir.entryInfoList();
		string src_file_path = srcFilepath.toStdString();

		QFileInfoList trimapList;
		QFileInfo trimapFileInfo;
		QDir trimapDir(trimapFilepath);
		trimapDir.setFilter(QDir::Files | QDir::NoSymLinks);
		trimapDir.setSorting(QDir::Name);
		trimapList = trimapDir.entryInfoList();
		string trimap_file_path = trimapFilepath.toStdString();

		Mat forsize;
		srcFileInfo = srcList.at(0);
		string forsizeFilename = src_file_path;
		forsizeFilename.append(srcFileInfo.fileName().toLocal8Bit().constData());
		forsize = imread(forsizeFilename);
		cv::resize(back_ground, back_ground, Size(forsize.cols, forsize.rows), 0, 0, CV_INTER_LINEAR);


		for (int i = 0; i < srcList.size(); ++i)
		{
			srcFileInfo = srcList.at(i);
			string srcFilename = src_file_path;
			srcFilename.append(srcFileInfo.fileName().toLocal8Bit().constData());
			src = imread(srcFilename);

			trimapFileInfo = trimapList.at(i);
			string trimapFilename = trimap_file_path;
			trimapFilename.append(trimapFileInfo.fileName().toLocal8Bit().constData());
			tmap = imread(trimapFilename);

			//trimap=Mat(tmap.rows,tmap.cols,CV_8UC1, cv::Scalar(0));
			//if (tmap.channels() == 3)
			//{
			//	cv::cvtColor(tmap, tmap, CV_RGB2GRAY);
			//}
			//else if (tmap.channels() == 1)
			//{
			//	tmap = tmap.clone();
			//}

			//for (int y = 0; y < tmap.rows; y++)
			//{
			//	for (int x = 0; x < tmap.cols; x++)
			//	{
			//		uchar v = tmap.at<uchar>(y, x);
			//		if (v < 20)	//if background
			//		{
			//			trimap.at<uchar>(y, x) = MASK_BACKGROUND;
			//		}
			//		else if (v >245)	//if foreground
			//		{
			//			trimap.at<uchar>(y, x) =  MASK_FOREGROUND;
			//		}
			//		else	// if unknown
			//		{
			//			trimap.at<uchar>(y, x)  = MASK_COMPUTE;
			//		}
			//	}
			//}


			Mat alphaMat;

			// 			cv::imshow("result", resultForeground);
			// 			cv::waitKey(0);
			string resultFilename = resultFilepath.toStdString() + "result";
			string alphaFilename = alphaFilepath.toStdString() + "alpha";
			resultFilename.append(srcFileInfo.fileName().toLocal8Bit().constData());
			alphaFilename.append(srcFileInfo.fileName().toLocal8Bit().constData());
			imwrite(resultFilename, mattingMethod(tmap, src, alphaMat,back_ground));
			imwrite(alphaFilename, alphaMat);
		}

		/*TCount = (double)cvGetTickCount()-TCount;
		totalTime = TCount/(cvGetTickFrequency()*1000000);
		ftime<<totalTime<<"s"<<endl;*/
	}

	QMessageBox::information(this, "Information", "Bat Matting finish!", QMessageBox::Ok);
}
/*
  convert images in opendir to a video( *.avi )
*/
void VideoEditingWindow::write_video()
{
	char *videoName = "VideoResult\\Result.avi";
	QString cur_dir = "./";
	QString filepath = QFileDialog::getExistingDirectory(this, "Get existing directory", cur_dir);
	filepath += "\\";
	QString videoFilepath = filepath;
	string vedioFilename = videoFilepath.toStdString();
	vedioFilename.append(videoName);

	//double fps=25.0;	//֡��

	if (!filepath.isEmpty())
	{
		QMessageBox::information(this, "Information", "Write video begin!", QMessageBox::Ok);

		QFileInfoList list;
		QFileInfo fileInfo;
		QDir dir(filepath);
		dir.mkdir("VideoResult");
		dir.setFilter(QDir::Files | QDir::NoSymLinks);
		dir.setSorting(QDir::Name);
		list = dir.entryInfoList();

		Mat frame;
		fileInfo = list.at(0);
		string file_path = filepath.toStdString();
		string filename = file_path;
		filename.append(fileInfo.fileName().toLocal8Bit().constData());
		frame = imread(filename.c_str());

		//CvVideoWriter * writer = 0;//��ʼ����Ƶд��		
		writer = VideoWriter(vedioFilename.c_str(), CV_FOURCC('X', 'V', 'I', 'D'), fps, Size(frame.cols, frame.rows));//��ʼ��������Ĭ��640*480��С����ɶ�ȡһ֡����ȡ��ԭʼ��С

		for (int i = 0; i < list.size(); ++i)
		{
			fileInfo = list.at(i);
			string filename = file_path;
			filename.append(fileInfo.fileName().toLocal8Bit().constData());
			frame = imread(filename.c_str());

			/************************************************************************/
			//cv::resize(frame,frame,Size(640,480),0,0,CV_INTER_LINEAR);
			/************************************************************************/


			if (!frame.empty())
			{
				/*imshow("1",frame);
				waitKey(1000);*/
				writer << frame;
			}
		}
		//reverse
		//for (int i = list.size()-1; i >=0; --i)
		//{		
		//	fileInfo = list.at(i);
		//	string filename=file_path;
		//	filename.append(fileInfo.fileName().toAscii().data());
		//	frame=imread(filename.c_str());

		//	if (!frame.empty())
		//	{
		//		/*imshow("1",frame);
		//		waitKey(1000);*/
		//		writer << frame;
		//	}
		//}
		if (writer.isOpened())
			writer.release();
		QMessageBox::information(this, "Information", "Write video finish!", QMessageBox::Ok);
	}

	//resultWidget = new myWidget(vedioFilename.c_str());
	//resultWidget->setWindowTitle("ResultVideo");
	//resultWidget->setVisible(true);
	//resultWidget->show();
}
/*
	convert alpha images in opendir to trimap images
*/
void VideoEditingWindow::alpha2Trimap()
{
	QString cur_dir = "./";
	QString alphaFilepath = QFileDialog::getExistingDirectory(this, "Get existing directory", cur_dir);
	alphaFilepath += "\\";
	QString trimapFilepath = alphaFilepath;
	trimapFilepath += "trimap\\";


	if (!alphaFilepath.isEmpty())
	{
		QMessageBox::information(this, "Information", "Alpha to trimap begin!", QMessageBox::Ok);

		QFileInfoList list;
		QFileInfo fileInfo;
		QDir dir(alphaFilepath);
		dir.mkdir("trimap");
		dir.setFilter(QDir::Files | QDir::NoSymLinks);
		dir.setSorting(QDir::Name);
		list = dir.entryInfoList();

		Mat alpha_frame;

		for (int i = 0; i<list.size(); ++i)
		{
			fileInfo = list.at(i);
			string alphaFilename = alphaFilepath.toStdString();
			alphaFilename.append(fileInfo.fileName().toLocal8Bit().constData());

			string trimapFilename = trimapFilepath.toStdString();
			trimapFilename.append(fileInfo.fileName().toLocal8Bit().constData());

			alpha_frame = imread(alphaFilename.c_str());

			if (!alpha_frame.empty() /*&& alpha_frame.channels()==1*/)
			{

				/*	if (alpha_frame.channels() == 3)
				{
				cv::cvtColor(alpha_frame, alpha_frame, CV_RGB2GRAY);
				}*/


				Mat trimap_frame = Mat(alpha_frame.rows, alpha_frame.cols, CV_8UC1, cv::Scalar(0));
				for (int i = 0; i<alpha_frame.cols; i++)
				{
					for (int j = 0; j<alpha_frame.rows; j++)
					{
						if (alpha_frame.at<uchar>(j, 3 * i) == 0)
						{
							trimap_frame.ptr<uchar>(j)[i] = MASK_BACKGROUND;
						}
						else if (alpha_frame.at<uchar>(j, 3 * i) == 255)
						{
							trimap_frame.ptr<uchar>(j)[i] = MASK_FOREGROUND;
						}
						else if (alpha_frame.at<uchar>(j, 3 * i)<255 && alpha_frame.at<uchar>(j, 3 * i)>0)
						{
							trimap_frame.ptr<uchar>(j)[i] = MASK_COMPUTE;
						}
					}
				}
				//Mat element = Mat::ones(8,8,CV_8UC1);
				//morphologyEx(trimap_frame,trimap_frame,cv::MORPH_CLOSE,element);	//������
				imwrite(trimapFilename, trimap_frame);
			}
		}
		QMessageBox::information(this, "Information", "Alpha to trimap finish!", QMessageBox::Ok);
	}
}
/*
	convert video( *.avi) in opendir to separate images
*/
void VideoEditingWindow::splitVideo()
{
	QString cur_dir = "./";
	QString fileName = QFileDialog::getOpenFileName(this, "Open File", cur_dir, "Vedio (*.avi)");
	std::string str = fileName.toStdString();
	const char* ch = str.c_str();
	VideoCapture splitcapture(ch);
	QString path;
	QFileInfo fi;
	fi = QFileInfo(fileName);
	path = fi.absolutePath();
	path += "/";
	QDir dir(path);
	dir.mkdir("Source");
	str = path.toStdString();
	const char* Pathch = str.c_str();

	int n = 0;
	char filename[200];
	Mat frame;
	for (;;) {
		splitcapture >> frame;
		if (frame.empty())
			break;
		sprintf(filename, "%sSource\\filename%.3d.jpg", Pathch, n++);
		imwrite(filename, frame);
	}
	QMessageBox::information(this, "Information", "Split Video finish!", QMessageBox::Ok);
}
/*
	caculte the gradient of curent frame,and save it to file
*/
void VideoEditingWindow::computeGradient()
{
	Mat forgray = curframe.clone();
	cvtColor(forgray, forgray, CV_RGB2GRAY);
	float maxValue = 0;
	float minValue = 0;
	double sum = 0;
	Mat gradientMat = Mat(forgray.rows, forgray.cols, CV_32FC1);

	string gradientFile = "./gradient.txt";
	ofstream fgradientout(gradientFile, ios::app);

	for (int y = 0; y < forgray.rows; ++y)
	{
		int isFinalY = (y != forgray.rows - 1);	//y�������һ�����أ���isFinalYΪ1��y�����һ�����أ���isFinalYΪ0

		for (int x = 0; x < forgray.cols; ++x)
		{
			int isFinalX = (x != forgray.cols - 1);		//trueΪ1��falseΪ0��x�������һ�����أ���isFinalXΪ1��x�����һ�����أ���isFinalXΪ0

			float deltaX = forgray.ptr<uchar>(y)[x + isFinalX] - forgray.ptr<uchar>(y)[x];	//�ұߵ����أ�x+1��-��ǰ	
			float deltaY = forgray.ptr<uchar>(y + isFinalY)[x] - forgray.ptr<uchar>(y)[x];	//�±ߵ����أ�y+1��-��ǰ	
			gradientMat.ptr<float>(y)[x] = sqrt(deltaX * deltaX + deltaY * deltaY);
			maxValue = maxValue > gradientMat.ptr<float>(y)[x] ? maxValue : gradientMat.ptr<float>(y)[x];	//����ݶ�
			minValue = minValue < gradientMat.ptr<float>(y)[x] ? minValue : gradientMat.ptr<float>(y)[x];	//��С�ݶ�
			sum += gradientMat.ptr<float>(y)[x];
			// 			fgradientout<<"maxMaybe:	"<<maxValue<<endl;
			// 			fgradientout<<"minMaybe:	"<<minValue<<endl;
		}
	}

	double average = sum / (forgray.rows*forgray.cols);
	// 	fgradientout<<"max:	"<<maxValue<<endl;
	// 	fgradientout<<"min:	"<<minValue<<endl;
	// 	fgradientout<<"average:	"<<average<<endl;
	QString aver = QString::number((int)average, 10);
	QMessageBox::information(this, "Information", "Compute gradient finish!", QMessageBox::Ok);
	QMessageBox::information(this, "Information", aver, QMessageBox::Ok);
}

void VideoEditingWindow::nextFrame()
{
	currentframePos++;
	if (currentframePos < totalFrameNumber)
	{
		currentFrame = &frame.at(currentframePos);
		if (currentFrame->ifKeyFrame == false)
		{
			ui_.SetKey->setChecked(false);
		}
		else if (currentFrame->ifKeyFrame == true)
		{
			ui_.SetKey->setChecked(true);
		}

		capture.set(CV_CAP_PROP_POS_FRAMES, currentframePos);
		capture >> curframe;
		if (!curframe.empty())
		{
			preprocessThread.setImage(curframe);
			cvtColor(curframe, tempFrame, CV_BGR2RGB);
			curImage = QImage((const unsigned char*)(tempFrame.data), tempFrame.cols, tempFrame.rows, QImage::Format_RGB888);
			ui_.videoFrame->clearRegion();
			ui_.videoFrame->setImage(&curImage);
			ui_.videoFrame->updateDisplayImage();
		}
	}
	else
	{
		currentframePos = totalFrameNumber - 1;
		QMessageBox::information(this, "Information", "Current frame is the last frame!", QMessageBox::Ok);
		timer->stop();
	}
	//currentframePos=capture.get(CV_CAP_PROP_POS_FRAMES);
	ui_.cur_frame_idx->setNum(currentframePos + 1);
	ui_.SelectArea->click();
	ui_.SetKey->setEnabled(false);
}

void VideoEditingWindow::pause()
{
	timer->stop();
	ui_.pushButton_previousframe->setEnabled(true);
	ui_.pushButton_nextframe->setEnabled(true);
	ui_.pushButton_play->setEnabled(true);
	ui_.pushButton_pause->setEnabled(false);
	ui_.toolsforstep1->setEnabled(true);
	ui_.SelectArea->click();

}

void VideoEditingWindow::play()
{
	timer->setInterval(delay);
	timer->start();
	//QMessageBox::information(this, "Information", "Video is on!",QMessageBox::Ok);
	ui_.pushButton_pause->setEnabled(true);
	ui_.pushButton_play->setEnabled(false);
	ui_.pushButton_previousframe->setEnabled(false);
	ui_.pushButton_nextframe->setEnabled(false);
	ui_.SetKey->setEnabled(false);
	ui_.toolsforstep1->setEnabled(false);
}

void VideoEditingWindow::preFrame()
{
	/*currentframePos=capture.get(CV_CAP_PROP_POS_FRAMES);
	long newcurrent =currentframePos-2;*/
	currentframePos--;
	if (currentframePos >= 0)
	{
		/*capture.set( CV_CAP_PROP_POS_FRAMES,(double)newcurrent);
		capture>>curframe;*/

		currentFrame = &frame.at(currentframePos);
		if (currentFrame->ifKeyFrame == false)
		{
			ui_.SetKey->setChecked(false);
		}
		else if (currentFrame->ifKeyFrame == true)
		{
			ui_.SetKey->setChecked(true);
		}

		capture.set(CV_CAP_PROP_POS_FRAMES, currentframePos);
		capture >> curframe;
		if (!curframe.empty())
		{
			preprocessThread.setImage(curframe);
			cvtColor(curframe, tempFrame, CV_BGR2RGB);
			curImage = QImage((const unsigned char*)(tempFrame.data), tempFrame.cols, tempFrame.rows, QImage::Format_RGB888);

			ui_.videoFrame->clearRegion();
			ui_.videoFrame->setImage(&curImage);
			ui_.videoFrame->updateDisplayImage();
		}
	}
	else
	{
		currentframePos = 0;
		QMessageBox::information(this, "Information", "Current frame is the first frame!", QMessageBox::Ok);
	}
	//currentframePos=capture.get(CV_CAP_PROP_POS_FRAMES);
	ui_.cur_frame_idx->setNum(currentframePos + 1);
	ui_.SelectArea->click();
	ui_.SetKey->setEnabled(false);
}

void VideoEditingWindow::turnToFrame(int curFrameNum)
{
	currentframePos = curFrameNum - 1;
	if (currentframePos >= 0 && currentframePos < totalFrameNumber)
	{
		currentFrame = &frame.at(currentframePos);
		if (currentFrame->ifKeyFrame == false)
		{
			ui_.SetKey->setChecked(false);
		}
		else if (currentFrame->ifKeyFrame == true)
		{
			ui_.SetKey->setChecked(true);
		}

		capture.set(CV_CAP_PROP_POS_FRAMES, currentframePos);
		capture >> curframe;
		if (!curframe.empty())
		{
			preprocessThread.setImage(curframe);
			cvtColor(curframe, tempFrame, CV_BGR2RGB);
			curImage = QImage((const unsigned char*)(tempFrame.data), tempFrame.cols, tempFrame.rows, QImage::Format_RGB888);

			ui_.videoFrame->clearRegion();
			ui_.videoFrame->setImage(&curImage);
			ui_.videoFrame->updateDisplayImage();
		}
	}
	else
	{
		QMessageBox::information(this, "Information", "Out of range !", QMessageBox::Ok);
	}
	//currentframePos=capture.get(CV_CAP_PROP_POS_FRAMES);
	ui_.cur_frame_idx->setNum(currentframePos + 1);
	ui_.SelectArea->click();
	ui_.SetKey->setEnabled(false);
}

void VideoEditingWindow::nextInitKey()
{
	set<int>::iterator it = initKeyframeNo.upper_bound(currentframePos);		//��ת����һ�����ڵ�ǰ֡��ŵĳ�ʼ�ؼ�֡
	if (it != initKeyframeNo.end())
	{
		currentframePos = *it;
		currentFrame = &frame.at(currentframePos);
		if (currentFrame->ifKeyFrame == false)
		{
			ui_.SetKey->setChecked(false);
		}
		else if (currentFrame->ifKeyFrame == true)
		{
			ui_.SetKey->setChecked(true);
		}

		capture.set(CV_CAP_PROP_POS_FRAMES, currentframePos);
		capture >> curframe;
		if (!curframe.empty())
		{
			preprocessThread.setImage(curframe);
			cvtColor(curframe, tempFrame, CV_BGR2RGB);
			curImage = QImage((const unsigned char*)(tempFrame.data), tempFrame.cols, tempFrame.rows, QImage::Format_RGB888);
			ui_.videoFrame->clearRegion();
			ui_.videoFrame->setImage(&curImage);
			ui_.videoFrame->updateDisplayImage();
		}
	}
	else
	{
		currentframePos = totalFrameNumber - 1;
		QMessageBox::information(this, "Information", "Current frame is the last frame!", QMessageBox::Ok);
		timer->stop();
	}
	//currentframePos=capture.get(CV_CAP_PROP_POS_FRAMES);
	ui_.cur_frame_idx->setNum(currentframePos + 1);
	ui_.SelectArea->click();
	ui_.SetKey->setEnabled(false);
}

void VideoEditingWindow::setKeyFrame()
{
	if (ui_.SetKey->isChecked())
	{
		currentFrame->ifKeyFrame = true;
		keyFrameNo.push_back(currentFrame->framePos);
		QImage trimapImage = ui_.videoFrame->getTrimap();
		unsigned *p = (unsigned*)trimapImage.bits();
		Mat trimapMat = Mat(trimapImage.height(), trimapImage.width(), CV_8UC1);

		for (int i = 0; i < trimapImage.width(); i++)
		{
			for (int j = 0; j < trimapImage.height(); j++)
			{
				unsigned* pixel = p + j*trimapImage.width() + i;
				if (*pixel == COMPUTE_AREA_VALUE)
				{
					trimapMat.ptr<uchar>(j)[i] = MASK_COMPUTE;
				}
				else if (*pixel == BACKGROUND_AREA_VALUE)
				{
					trimapMat.ptr<uchar>(j)[i] = MASK_BACKGROUND;
				}
				else if (*pixel == FOREGROUND_AREA_VALUE)
				{
					trimapMat.ptr<uchar>(j)[i] = MASK_FOREGROUND;
				}
			}
		}
		currentFrame->trimap = trimapMat.clone();

	}
	else if (!ui_.SetKey->isChecked())
	{
		currentFrame->ifKeyFrame = false;
		keyFrameNo.remove(currentFrame->framePos);
		//currentFrame->trimap = Mat((curframe).rows,(curframe).cols,CV_8UC1);		//û����費��Ҫ	
	}
}

void VideoEditingWindow::initKeyframe()
{
	FrameDif framedif;
	framedif.initKeyFrame(initKeyframeNo, capture, totalFrameNumber);
	ui_.NextInitKey->setEnabled(true);
}

void VideoEditingWindow::trimapInterpolation()
{
	InterpolationTrimap trimapInter;
	if (keyFrameNo.size()<2)
	{
		QMessageBox::information(this, "Information", "Please set some key frames first , at least two key frames!", QMessageBox::Ok);
		return;
	}
	QMessageBox::information(this, "Information", "Trimap Interpolation begin!", QMessageBox::Ok);
	keyFrameNo.sort();	//�ؼ�֡��Ŵ�С��������
	Size size = Size(curframe.rows, curframe.cols);
	trimapInter.interpolationTrimap(frame, keyFrameNo, capture, size, currentfilePath);
	//list<long>::iterator it = keyFrameNo.begin();		//����
	//list<long>::reverse_iterator rit = keyFrameNo.rbegin();		//����

	//Mat frameMat,pregray,gray,forwardFlow,backwardFlow;	//for computing optical flow
	//Mat flowframe ;		//image predicted by optical flow
	///*Mat flowframe_resampled;*/
	//char filename[200];

	//vector<FlowError> flowError;
	//flowError.resize(totalFrameNumber);

	//for (int i=0;i<totalFrameNumber;i++)	//��ʼ�����
	//{
	//	flowError.at(i).framePos = i;
	//	flowError.at(i).forwardErrorMap = Mat::zeros(size,CV_32FC1);		
	//	flowError.at(i).backwardErrorMap = Mat::zeros(size,CV_32FC1);
	//	flowError.at(i).forwardAccumulatedError = Mat::zeros(size,CV_32FC1);
	//	flowError.at(i).backwardAccumulatedError = Mat::zeros(size,CV_32FC1);
	//}

	//while(*it != keyFrameNo.back())		//����ֱ��it�������һ��Ԫ��ֹͣѭ��
	//{
	//	long begin = *it,end =*(++it);	//��ȡ�������ڵĹؼ�֡��ţ��õ�һ����ֵ����
	//	
	//	for (long i= begin; i<end;++i)		
	//	{
	//		//QMessageBox::information(this, "Information", QString::number(i, 10),QMessageBox::Ok);
	//		//curPos = frame.at(i).framePos;
	//		capture.set(CV_CAP_PROP_POS_FRAMES,i);
	//		capture>>frameMat;
	//		cvtColor(frameMat,gray,COLOR_BGR2GRAY);			
	//		
	//		flowframe = Mat(frameMat.rows,frameMat.cols,frameMat.type());	//image predicted by optical flow
	//		
	//		if (i!=begin)
	//		{
	//			if (pregray.data)
	//			{
	//				for (int y = 0; y < frameMat.rows; y++)
	//				{
	//					for (int x = 0; x < frameMat.cols; x++)
	//					{
	//						//frame.at(i).trimap.ptr<uchar>(y)[x] = 0;		
	//						flowframe.ptr<uchar>(y)[x*3] = 0;			//b
	//						flowframe.ptr<uchar>(y)[x*3+1] = 0;		//g
	//						flowframe.ptr<uchar>(y)[x*3+2] = 0;		//r
	//					}
	//				}

	//				/*{
	//					calcOpticalFlowFarneback(pregray, gray, forwardFlow, 0.5, 3, 15, 3, 5, 1.2, 0);
	//				}*/

	//					//�����ܼ�����
	//				Ptr<DenseOpticalFlow> tvl1 = createOptFlow_DualTVL1();
	//				tvl1->calc(pregray, gray, forwardFlow);
	//				
	//				for (int y = 0; y < forwardFlow.rows; y++)
	//				{
	//					for (int x = 0; x < forwardFlow.cols; x++)
	//					{				
	//						const Point2f& fxy = forwardFlow.at<Point2f>(y, x);
	//						if (isFlowCorrect(fxy))
	//						{
	//							int newy = cvRound(y+fxy.y) > 0 ? cvRound(y+fxy.y) : 0;
	//							newy = newy <  forwardFlow.rows? newy : (forwardFlow.rows-1);
	//							int newx = cvRound(x+fxy.x) > 0 ? cvRound(x+fxy.x) : 0;
	//							newx = newx <  forwardFlow.cols? newx : (forwardFlow.cols-1);

	//							frame.at(i).trimap.ptr<uchar>(newy)[newx] = frame.at(i-1).trimap.ptr<uchar>(y)[x];		//��������������ɵ���Ϊ��ʼֵ�������Ϸ���������ɵĽ��и���								
	//							/************************************************************************/
	//							/* ���Ǽ�����̬ѧ���㣬��������ı�ʾһ�����С����ֵ����������Щ*/
	//							/************************************************************************/
	//							

	//							//����Ԥ��õ���ͼ��
	//							flowframe.ptr<uchar>(newy)[newx*3] = frameMat.ptr<uchar>(y)[x*3]; 			//������ˣ�Ӧ���и�preframeMat�����°�
	//							flowframe.ptr<uchar>(newy)[newx*3+1] = frameMat.ptr<uchar>(y)[x*3+1];	
	//							flowframe.ptr<uchar>(newy)[newx*3+2] = frameMat.ptr<uchar>(y)[x*3+2];	

	//							if (i!=begin+1)
	//							{
	//								flowError.at(i).forwardAccumulatedError.ptr<float>(newy)[newx] = flowError.at(i-1).forwardAccumulatedError.ptr<float>(y)[x];
	//							}								
	//						}
	//					}
	//				}	
	//				/*Size dstSize(flowframe.cols,flowframe.rows);
	//				cv::resize(flowframe,flowframe_resampled,dstSize);
	//				imshow("resampled",flowframe_resampled);
	//				waitKey(20);*/
	//				/*imshow("flowframe",flowframe);
	//				waitKey(20);
	//				imshow("mat",frameMat);
	//				waitKey(20);*/

	//				/*double error_value;*/

	//				double r1,g1,b1;
	//				double r2,g2,b2;

	//				/*int count=0;*/
	//				for (int y=0; y<frameMat.rows;y++)		
	//				{
	//					for (int x=0; x<frameMat.cols;x++)
	//					{
	//						b1 = (double)frameMat.ptr<uchar>(y)[x*3];
	//						g1 = (double)frameMat.ptr<uchar>(y)[x*3+1];
	//						r1 = (double)frameMat.ptr<uchar>(y)[x*3+2];

	//						b2 = (double)flowframe.ptr<uchar>(y)[x*3];
	//						g2 = (double)flowframe.ptr<uchar>(y)[x*3+1];
	//						r2 = (double)flowframe.ptr<uchar>(y)[x*3+2];

	//						/*frame.at(i).forwardErrorMap.at<float>(y,x)=sqrt((double)(frameMat.ptr<uchar>(y)[x*3] - flowframe.ptr<uchar>(y)[x*3])*(frameMat.ptr<uchar>(y)[x*3] - flowframe.ptr<uchar>(y)[x*3])+
	//																							(frameMat.ptr<uchar>(y)[x*3+1] - flowframe.ptr<uchar>(y)[x*3+1])*(frameMat.ptr<uchar>(y)[x*3+1] - flowframe.ptr<uchar>(y)[x*3+1])+
	//																							(frameMat.ptr<uchar>(y)[x*3+2] - flowframe.ptr<uchar>(y)[x*3+2])*(frameMat.ptr<uchar>(y)[x*3+2] - flowframe.ptr<uchar>(y)[x*3+2]));*/
	//						//frame.at(i).forwardErrorMap.at<float>(y,x)=sqrt((b1 - b2) * (b1 - b2) + (g1 - g2) * (g1 - g2) + (r1 - r2) * (r1 - r2));	//�к��еķ��������
	//						//error_value=frame.at(i).forwardErrorMap.at<float>(y,x);
	//						flowError.at(i).forwardErrorMap.ptr<float>(y)[x] = sqrt((b1 - b2) * (b1 - b2) + (g1 - g2) * (g1 - g2) + (r1 - r2) * (r1 - r2));		//���������Ԥ��ͼ����ʵ��ͼ�����ص�ŷ�Ͼ���
	//					/*	error_value = frame.at(i).forwardErrorMap.ptr<float>(y)[x];
	//						if (error_value<30)
	//						{
	//							count++;
	//						}*/

	//						if (i==begin+1)
	//						{
	//							flowError.at(i).forwardAccumulatedError.ptr<float>(y)[x] = flowError.at(i).forwardErrorMap.ptr<float>(y)[x];
	//						}
	//						else
	//						{
	//							flowError.at(i).forwardAccumulatedError.ptr<float>(y)[x]+=flowError.at(i).forwardErrorMap.ptr<float>(y)[x];
	//						}
	//					}
	//				}
	//			}
	//		}
	//		Mat element = Mat::ones(8,8,CV_8UC1);
	//		morphologyEx(frame.at(i).trimap,frame.at(i).trimap,cv::MORPH_CLOSE,element);	//������
	//			sprintf(filename,"G:\\Liya\\Task\\video matting\\Data\\Duck\\Trimap\\forwardtrimap%.4d.jpg",i);
	//		imwrite(filename,frame.at(i).trimap);
	//		std::swap(pregray, gray);	//��һ�μ���ʱ����ǰ֡�����һ֡	
	//	}
	//}

	////���������������
	//vector<Mat> backwardTrimap;


	//while(*rit != keyFrameNo.front())		//backward
	//{
	//	long rbegin = *rit,rend =*(++rit);
	//	int interframeNum = rbegin-rend-1;
	//	backwardTrimap.resize(interframeNum+1);
	//	backwardTrimap.at(rbegin-rend-1) = frame.at(rbegin).trimap.clone();
	//	for (long i= rbegin; i>rend; --i)
	//	{
	//		//QMessageBox::information(this, "Information", QString::number(i, 10),QMessageBox::Ok);
	//		capture.set(CV_CAP_PROP_POS_FRAMES,i);
	//		capture>>frameMat;
	//		cvtColor(frameMat,gray,COLOR_BGR2GRAY);

	//		flowframe = Mat(frameMat.rows,frameMat.cols,frameMat.type());				

	//		if (i!=rbegin)
	//		{
	//			backwardTrimap.at(i-rend-1) = Mat(frameMat.rows,frameMat.cols,CV_8UC1, cv::Scalar(0));
	//			if (pregray.data)
	//			{
	//				for (int y=0;y<frameMat.rows;y++)
	//				{
	//					for (int x=0;x<frameMat.cols;x++)
	//					{
	//						flowframe.ptr<uchar>(y)[x*3] = 0;			//b
	//						flowframe.ptr<uchar>(y)[x*3+1] = 0;		//g
	//						flowframe.ptr<uchar>(y)[x*3+2] = 0;		//r
	//					}
	//				}
	//				
	//				Ptr<DenseOpticalFlow> tvl1 = createOptFlow_DualTVL1();
	//				tvl1->calc(pregray, gray, backwardFlow);					

	//				for(int y=0; y<backwardFlow.rows; y++)
	//				{
	//					for (int x=0; x<backwardFlow.cols; x++)
	//					{
	//						const Point2f& fxy = backwardFlow.at<Point2f>(y,x);
	//						if (isFlowCorrect(fxy))
	//						{
	//							int newy = cvRound(y+fxy.y) > 0 ? cvRound(y+fxy.y) : 0;
	//							newy = newy <  backwardFlow.rows? newy : (backwardFlow.rows-1);
	//							int newx = cvRound(x+fxy.x) > 0 ? cvRound(x+fxy.x) : 0;
	//							newx = newx <  backwardFlow.cols? newx : (backwardFlow.cols-1);

	//							backwardTrimap.at(i-rend-1).ptr<uchar>(newy)[newx] = backwardTrimap.at(i-rend-1+1).ptr<uchar>(y)[x];

	//							//����Ԥ��õ���ͼ��
	//							flowframe.ptr<uchar>(newy)[newx*3] = frameMat.ptr<uchar>(y)[x*3];
	//							flowframe.ptr<uchar>(newy)[newx*3+1] = frameMat.ptr<uchar>(y)[x*3+1];
	//							flowframe.ptr<uchar>(newy)[newx*3+2] = frameMat.ptr<uchar>(y)[x*3+2];

	//							if (i!=rbegin-1)
	//							{
	//								flowError.at(i).backwardAccumulatedError.ptr<float>(newy)[newx] = flowError.at(i+1).backwardAccumulatedError.ptr<float>(y)[x];
	//							}
	//						}
	//					}
	//				}

	//				double r1,g1,b1;
	//				double r2,g2,b2;

	//				for (int y=0; y<frameMat.rows; y++)
	//				{
	//					for(int x=0; x<frameMat.cols; x++)
	//					{
	//						b1 = (double)frameMat.ptr<uchar>(y)[x*3];
	//						g1 = (double)frameMat.ptr<uchar>(y)[x*3+1];
	//						r1 = (double)frameMat.ptr<uchar>(y)[x*3+2];

	//						b2 = (double)flowframe.ptr<uchar>(y)[x*3];
	//						g2 = (double)flowframe.ptr<uchar>(y)[x*3+1];
	//						r2 = (double)flowframe.ptr<uchar>(y)[x*3+2];

	//						flowError.at(i).backwardErrorMap.ptr<float>(y)[x] = sqrt((b1 - b2) * (b1 - b2) + (g1 - g2) * (g1 - g2) + (r1 - r2) * (r1 - r2));		//���������Ԥ��ͼ����ʵ��ͼ�����ص�ŷ�Ͼ���

	//						if (i==rbegin - 1)	//����ǹؼ�֡����һ֡���ۻ������ǵ�ǰ���
	//						{
	//							flowError.at(i).backwardAccumulatedError.ptr<float>(y)[x] = flowError.at(i).backwardErrorMap.ptr<float>(y)[x];
	//						}
	//						else
	//						{
	//							flowError.at(i).backwardAccumulatedError.ptr<float>(y)[x]+=flowError.at(i).backwardErrorMap.ptr<float>(y)[x];
	//						}
	//					}
	//				}

	//				/*imshow("before trimap",frame.at(i).trimap);
	//				waitKey(20);*/

	//				for (int y=0; y<frame.at(i).trimap.rows; y++)
	//				{
	//					for (int x=0; x<frame.at(i).trimap.cols; x++)
	//					{
	//						if (flowError.at(i).backwardAccumulatedError.ptr<float>(y)[x] < flowError.at(i).forwardAccumulatedError.ptr<float>(y)[x])	//Ĭ��ֵΪ����Ԥ����������������������С�������Ϊ����Ԥ����
	//						{
	//							frame.at(i).trimap.ptr<uchar>(y)[x] = backwardTrimap.at(i-rend-1).ptr<uchar>(y)[x];
	//						}
	//						else if(abs(flowError.at(i).backwardAccumulatedError.ptr<float>(y)[x] - flowError.at(i).forwardAccumulatedError.ptr<float>(y)[x])<FLT_EPSILON)	//�������ȡ��ֵ???
	//						{
	//							frame.at(i).trimap.ptr<uchar>(y)[x] = (frame.at(i).trimap.ptr<uchar>(y)[x]+backwardTrimap.at(i-rend-1).ptr<uchar>(y)[x])/2;
	//						}
	//					}
	//				}

	//				/*imshow("after trimap",frame.at(i).trimap);
	//				waitKey(20);*/
	//			}
	//		}
	//		//////////////////////////////////////////////////////////////////////////
	//		Mat element = Mat::ones(8,8,CV_8UC1);
	//		morphologyEx(backwardTrimap.at(i-rend-1),backwardTrimap.at(i-rend-1),cv::MORPH_CLOSE,element);	//������
	//		sprintf(filename,"G:\\Liya\\Task\\video matting\\Data\\Duck\\Trimap\\backwardtrimap%.4d.jpg",i);
	//		imwrite(filename,backwardTrimap.at(i-rend-1));

	//		morphologyEx(frame.at(i).trimap,frame.at(i).trimap,cv::MORPH_CLOSE,element);	//������
	//		sprintf(filename,"G:\\Liya\\Task\\video matting\\Data\\Duck\\Trimap\\finaltrimap%.4d.jpg",i);
	//		imwrite(filename,frame.at(i).trimap);
	//		std::swap(pregray,gray);//��һ�μ���ʱ����ǰ֡�����һ֡	
	//	}
	//}



	//keyFrameNo.reverse();	//���򣬴Ӵ�С����
	QMessageBox::information(this, "Information", "Trimap Interpolation finish!", QMessageBox::Ok);
}

void VideoEditingWindow::mattingVideo()
{
	QMessageBox::information(this, "Information", "Matting Video begin!", QMessageBox::Ok);
	char fileName[200], alphaName[200];
	Mat srcMat;
	for (int i = 0; i < totalFrameNumber; i++)
	{
		capture.set(CV_CAP_PROP_POS_FRAMES, i);
		capture >> srcMat;

		Mat alphaMat;

		const char* filePathCh = currentfilePath.c_str();
		sprintf(fileName, "%sResult/compositeResult%.4d.jpg", filePathCh, i);
		sprintf(alphaName, "%sAlpha/Aplha%.4d.jpg", filePathCh, i);
		//sprintf(fileName,"G:\\Liya\\Task\\video matting\\Data\\Result\\compositeResult%.4d.jpg",i);
		imwrite(fileName, mattingMethod(frame.at(i).trimap, srcMat, alphaMat, back_ground));
		imwrite(alphaName, alphaMat);
	}
	QMessageBox::information(this, "Information", "Matting Video finish!", QMessageBox::Ok);
}

void VideoEditingWindow::changeBackground()
{
	QString cur_dir = "G:\\Liya\\Task\\video matting\\Data\\Picture";
	QString fileName = QFileDialog::getOpenFileName(this, "Open File", cur_dir, "Images (*.bmp *.gif *.jpg *.jpeg *.png *.tiff)");
	std::string str = fileName.toStdString();
	const char* ch = str.c_str();
	if (ch)
	{
		back_ground = imread(ch);
	}
	if (back_ground.data == NULL) {
		cerr << "Cann't open composite image." << endl;
	}
	cv::resize(back_ground, back_ground, Size(curframe.cols, curframe.rows), 0, 0, CV_INTER_LINEAR);
}

void VideoEditingWindow::increaseWidth(bool isIncrease)
{
	int v = ui_.kernelSizeSlider->value();
	int newV = isIncrease ? v + 1 : v - 1;
	ui_.kernelSizeSlider->setValue(newV);
}

void VideoEditingWindow::setGaussianKernelSize(int kernel)
{
	preprocessor.setGaussianKernelSize(kernel / 10.0f);
	updateMidImage();
}

void VideoEditingWindow::grabcutIteration()
{
	preprocessThread.GrabCutItera();
}

void VideoEditingWindow::showTrimap()
{
	QImage trimapImage = ui_.videoFrame->getTrimap();
	unsigned *p = (unsigned*)trimapImage.bits();
	Mat trimap = Mat(trimapImage.height(), trimapImage.width(), CV_8UC1);

	for (int i = 0; i < trimapImage.width(); i++)
	{
		for (int j = 0; j < trimapImage.height(); j++)
		{
			unsigned* pixel = p + j*trimapImage.width() + i;
			if (*pixel == COMPUTE_AREA_VALUE)
			{
				trimap.ptr<uchar>(j)[i] = MASK_COMPUTE;
			}
			else if (*pixel == BACKGROUND_AREA_VALUE)
			{
				trimap.ptr<uchar>(j)[i] = MASK_BACKGROUND;
			}
			else if (*pixel == FOREGROUND_AREA_VALUE)
			{
				trimap.ptr<uchar>(j)[i] = MASK_FOREGROUND;
			}
		}
	}

	imshow("trimap", trimap);
	waitKey(50);
}

void VideoEditingWindow::showKeyFrameNO()
{
	QString keyNO = "Key Frame : ";
	keyFrameNo.sort();
	for (list<int>::iterator it = keyFrameNo.begin(); it != keyFrameNo.end(); it++)
	{
		keyNO += QString::number(*it, 10);
		keyNO += ",";
	}
	QMessageBox::information(this, "Information", keyNO, QMessageBox::Ok);
}

void VideoEditingWindow::mattingFrame() //�Ե���֡��ͼ���Ա�۲쵱ǰtrimap��ͼ���
{
	QImage trimapImage = ui_.videoFrame->getTrimap();
	unsigned *p = (unsigned*)trimapImage.bits();
	Mat trimap = Mat(trimapImage.height(), trimapImage.width(), CV_8UC1);

	for (int i = 0; i<trimapImage.width(); i++)
	{
		for (int j = 0; j<trimapImage.height(); j++)
		{
			unsigned* pixel = p + j*trimapImage.width() + i;
			if (*pixel == COMPUTE_AREA_VALUE)
			{
				trimap.ptr<uchar>(j)[i] = MASK_COMPUTE;
			}
			else if (*pixel == BACKGROUND_AREA_VALUE)
			{
				trimap.ptr<uchar>(j)[i] = MASK_BACKGROUND;
			}
			else if (*pixel == FOREGROUND_AREA_VALUE)
			{
				trimap.ptr<uchar>(j)[i] = MASK_FOREGROUND;
			}
		}
	}

	imshow("trimap", trimap);
	waitKey(20);

	Mat alphaMat;

	imshow("Matting Result", mattingMethod(trimap, curframe, alphaMat, back_ground));
	imshow("Alpha", alphaMat);
	/*BayesianMatting matting(curframe,trimap);
	matting.Solve();
	matting.Composite(back_ground, &compositeResult);
	compositeResult.convertTo(compositeResult,CV_8UC3,255.0);*/

	//double minVal, maxVal;
	//minMaxLoc(resultForeground, &minVal, &maxVal); //find minimum and maximum intensities
	//resultForeground.convertTo(resultForeground,CV_8UC3,255.0/(maxVal - minVal), -minVal * 255.0/(maxVal - minVal));

	//imwrite("res.jpg",compositeResult);
	/*mat2QImage(compositeResult,resultQImage);
	ResVideo->setResultImage(resultQImage);*/
}

void VideoEditingWindow::updateMidImage()
{
	preprocessThread.setDirty();
}
void VideoEditingWindow::refresh()
{
	ui_.videoFrame->updateTrimap(preprocessThread.getTrimap());
	ui_.videoFrame->updateDisplayImage();
	ui_.SetKey->setEnabled(true);
	/*if (!mattingThread.isThreadWorking())
	{
	compute();
	}*/
}
void VideoEditingWindow::cutInteract()
{
	if (curImage.size().isEmpty() || ui_.videoFrame->getMask().isNull())
	{
		return;
	}
	preprocessThread.interactCut(ui_.videoFrame->getMask());
}
void VideoEditingWindow::cutSelect()
{
	if (curImage.size().isEmpty() || !ui_.videoFrame->getHasRegion())
	{
		return;
	}
	preprocessThread.selectCut(ui_.videoFrame->getRect());
}