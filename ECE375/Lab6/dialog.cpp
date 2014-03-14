#include "dialog.h"
#include "ui_dialog.h"
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#define MASK_BUTTON_Z 0x01
#define MASK_BUTTON_C 0x02
#define MASK_ACCEL_X 0x0C
#define MASK_ACCEL_Y 0x40
#define MASK_ACCEL_Z 0xC0

#define BUTTON_Z(a) !(a & MASK_BUTTON_Z)
#define BUTTON_C(a) !((a & MASK_BUTTON_C) >> 1)

#define ACCEL_X(a, b) ((a <<2) | ((b & MASK_ACCEL_X) >> 2))
#define ACCEL_Y(a, b) ((a <<2) | ((b & MASK_ACCEL_Y) >> 4))
#define ACCEL_Z(a, b) ((a <<2) | ((b & MASK_ACCEL_Z) >> 6))

#define X_CENTER 133
#define Y_CENTER 128

#define DX(a) ((a - X_CENTER) / 10)
#define DY(a) ((Y_CENTER - a) / 10)
//Skeleton Code for ECE 375 Lab 6 - Intel Atom Lab

int file;
char filename[20];

//Initialize the starting point
QPoint hold = QPoint(150, 150);

QPen pen = QPen();

void openfile()
{
    //open the device file
    snprintf(filename, 19, "/dev/i2c-1");
    file = open(filename, O_RDWR);

    if (file < 0)
    {
        printf("Failed to open i2c device.\n");
        exit(1);
    }

    //specify what device address you want to communicate

    int addr = 0x52;	 //address for nunchuck

		//Access the I2C slave

    if(ioctl(file, I2C_SLAVE, addr) < 0)
    {
        printf("Failed to acquire bus access to slave.\n");
        printf("error..., something is wrong %s\n", strerror(errno));
        exit(1);
    }

		//TODO: write initialization data to the nunchuk.
		//if it returns with error (-1) exit with status 1
	//Write init registers
    if(i2c_smbus_write_byte_data(file, 0xF0, 0x55) < 0) {
    	printf("Failed to initialize nunchuck with 0xFE, 0x55");
	exit(1);
    }

    if(i2c_smbus_write_byte_data(file, 0xFB, 0x00) < 0) {
    	printf("Failed to initialize nunchuck with 0xFB, 0x00");
	exit(1);
    }
} 

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    openfile();
    timer= new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(repaint()));

    timer->start(100);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::paintEvent(QPaintEvent *e)
{
    unsigned char data[6];

    QPainter painter(this);
    QWidget::setAttribute( Qt::WA_OpaquePaintEvent, true );
	// Request data
       if(i2c_smbus_write_byte(file, 0x00) == -1)
        {
            printf("error..., something is wrong %s\n", strerror(errno));
            exit(1);
        }
	// Read in 6 bytes of data from nunchuck
	if(i2c_smbus_read_i2c_block_data(file, 0x00, 6, data) < 0) {
		printf("something went wrong %s/n", strerror(errno));
		exit(1);
	}

		//TODO: Convert data bytes into analog X/Y, accelerometer X/Y/Z, button X/Z
	// Send raw data through masking £define
	int accelX = ACCEL_X(data[2], data[5]);
	int accelY = ACCEL_Y(data[3], data[5]);
	int accelZ = ACCEL_Z(data[4], data[5]);

	int analogX = data[0];
	int analogY = data[1];

	int buttonZ = BUTTON_Z(data[5]);
	int buttonC = BUTTON_C(data[5]);
     // Output from the nunchuck for reference //

				//TODO: Print output of analog stick and X, Y, Z axes
        printf("Analog X: %d\n", analogX);
	printf("Analog Y: %d\n", analogY);

	printf("Accel X: %d\n", accelX);
	printf("Accel Y: %d\n", accelY);
	printf("Accel Z: %d\n", accelZ);
				//TODO: Print output of buttons Z and C
	printf("Z Button: %d\n", buttonZ);
	printf("C Button: %d\n", buttonC);

	//When drawing pressing button Z should exit
	if(buttonZ) {
		painter.eraseRect(e->rect());
		return;
	}
	//If C is depressed then change the color based on
	//accelerometer
	if(buttonC) {
		pen.setColor(QColor(accelX % 255, accelY % 255, accelZ % 255));
	}
	painter.setPen(pen);
		//Calculate the next point based on analog stick data.	
		//You might need to convert some values.  Experiment...


		//Draw a line between the previous point and the new point.
	//Determine point to goto
	QPoint point = QPoint(DX(analogX), DY(analogY)) + hold;	
	//Stay in bounds of drawing rectangle
	if(point.x() > e->rect().right()) {
		point.setX(e->rect().right());
	}
	if(point.x() < e->rect().left()) {
		point.setX(e->rect().left());
	}
	if(point.y() > e->rect().bottom()) {
		point.setY(e->rect().bottom());
	}
	if(point.y() < e->rect().top()) {
		point.setY(e->rect().top());
	}
	//Draw line and update current point
	painter.drawLine(hold, point);
	hold = point;
}

