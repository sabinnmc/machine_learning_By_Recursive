include env.mk

INC = -I/usr/include -I./unzip/include -I/usr/include -I./hailo -I/usr/local/include/opencv4
LIB =  -L./unzip/lib -lunzip -L/usr/local/lib -L/usr/lib -lz -ljpeg -lrt -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs  -lopencv_video -lmd -lasound -lboost_system -lboost_filesystem -lhailort

CFLAGS = -pthread -std=c++17 $(OPT) -Wall -Werror -c $(INC)
LFLAGS = -pthread -std=c++17 $(OPT)

#DEPS = YOLOv3_gen.h
OBJS = BoardCtrl.o Global.o Param.o main.o SerialCtrl.o SerialCommand.o LinuxUtility.o Inference.o Sysdef.o ErrorManager.o Recoder.o HailoDetect.o hailo/yolo_output.o hailo/yolo_postprocess.o CanCtrl.o CanCommand.o TransForm.o flashmem.o Display.o Capture.o Application.o ExternalMemoryUtility.o testing.o

TGT  = ./bin/Has2App

%.o: %.cpp $(DEPS)
	$(GPP) $(CFLAGS) -o $@ $<

all : $(OBJS)
	$(GPP) $(LFLAGS) $(OBJS) -o $(TGT) $(LIB)

clean:
	rm -f *.o hailo/*.o $(TGT)
