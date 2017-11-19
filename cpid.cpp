/*
  ┌──┬───────────────────────────┬──┐
 ┌┤  │  CPID Temperature Control │  ├┐
 └┤  │  Nick Charron - 2017      │  ├┘
 ┌┤  │  Rice Univ - Huang Lab    │  ├┐
 └┤  ├───────────────────────────┤  ├┘
  └──┴───────────────────────────┴──┘
*/

#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <curses.h>

using namespace std;

class front_end{
	public:
	WINDOW* STATS;
	WINDOW* SETWIN;
	double temp, rh; 	
	
	void curses_init(){
		const char* strings[] = {"PID CONTROL",\
				"TEMP:","SETPOINT:","POWER:",\
				"TIME:"};
		initscr();
    	start_color();
    	init_pair(1,4,0);
    	init_pair(2,1,0);
    	cbreak();
    	noecho();
    	curs_set(0);
    	nodelay(stdscr,TRUE);
    	keypad(stdscr,TRUE);

	STATS = newwin(LINES/2,COLS/2,LINES/4,COLS/4);
	SETWIN= newwin(3,COLS,LINES-4,1);
	wborder(STATS,0,0,0,0,0,0,0,0);
	mvwaddstr(STATS,1,(COLS/2 - strlen(strings[0]))/2,strings[0]);
	for(int i=1;i<4;i++){	
		mvwaddstr(STATS,i*(LINES/10),2,strings[i]);
	}	
	refresh();
	wrefresh(STATS);
	}

	void update(char* temp_str,char* setpoint,char* power){
		const char* strings[] = {"PID CONTROL",\
				"TEMP:","SETPOINT:","POWER:",\
				"TIME:"};
		mvwaddstr(STATS,LINES/10,16,temp_str);
		mvwaddstr(STATS,2*LINES/10,16,setpoint);
	 	mvwaddstr(STATS,3*LINES/10,16,power);
		for(int i=1;i<4;i++){	
			mvwaddstr(STATS,i*(LINES/10),2,strings[i]);
		}
		refresh();
		wborder(STATS,0,0,0,0,0,0,0,0);
		wrefresh(STATS);
	}
	
	char* setpoint(){
		const char* cmd = "ENTER SETPOINT: ";
		char *set = (char *)malloc(32*sizeof(char));
		char *sendstring = (char*)malloc(32*sizeof(char));
		set[0]='\0';
		mvwaddstr(SETWIN,1,1,cmd);
		refresh();
		wrefresh(SETWIN);
		echo();
		wgetstr(SETWIN,set);
		strcat(sendstring,"<s_");
		strcat(sendstring,set);
		strcat(sendstring,">");
		noecho();
		werase(SETWIN);
		refresh();
		wrefresh(SETWIN);
		return sendstring;
	}
};

class data_line{
	public:
	int fd;
	char buffer[32];
	double temp, power;
	
	void open_port(const char* 
		PORTNAME, tcflag_t OOPTS){
    	int descriptor;
    	descriptor = open(PORTNAME, OOPTS);
    	if(descriptor == -1){
        	perror("\nUnable to establish port connection.\n");
        	exit(0);
    	}
    	else{
        	fcntl(descriptor, F_SETFL, 0);
    	}
    	fd = descriptor;
	}

	void port_conf_8N1(int fd, int IBAUD, int OBAUD, struct termios options){
    	 tcgetattr(fd,&options);
         cfsetispeed(&options, B9600);
         cfsetospeed(&options, B9600);
         options.c_cflag |= (CLOCAL | CREAD);
         options.c_cflag &= ~PARENB;
         options.c_cflag &= ~CSTOPB;
         options.c_cflag &= ~CSIZE;
         options.c_cflag |= CS8;
         options.c_lflag |= ICANON;
         tcsetattr(fd,TCSANOW, &options);
	}
};

//      ┌─────┬──┬─────┐
//      │┌┬───┴──┴───┬┐│
//      ├┤│   MAIN   │├┤
//      │└┴───┬──┬───┴┘│
//      └─────┴──┴─────┘

int main(int argc, char** argv){

	char ch;
	const char* PORTNAME = "/dev/tty.usbmodem1411";	
	int bytesread, steps;
	double temp;
	char* temp_str;
	char*  pow_str;
	struct termios options;
	char set[3] = {'N','/','A'};
	data_line DATA;
	
//    ┌────────────────────────┐
//    │  INITIATE CONNECTION   │
//    └────────────────────────┘ 

	cout << "\nConnecting...";
	DATA.open_port(PORTNAME, O_RDWR | O_NOCTTY | O_NDELAY);
	fflush(stdout);
	usleep(2500000);
	cout << "success\n"; 
	cout << "Configuring port settings...";
	DATA.port_conf_8N1(DATA.fd, B9600, B9600, options);
	fflush(stdout);
	usleep(2500000);
	cout << "success\n";
	
//    ┌───────────────────┐
//    │   PRE-LOOP PREP   │     	
//    └───────────────────┘ 

	steps = 0;
	front_end FRONT;
	FRONT.curses_init();
	char* sendstring = (char*)malloc(32*sizeof(char));
	char* str = (char*)malloc(32*sizeof(char));
	char setpoint[32];

//    ┌────────────────┐
//    │   MAIN LOOP    │
//    └────────────────┘

	while(1){
		ch = getch();
		if(ch == 'q'){endwin();close(DATA.fd);exit(1);}
		if(ch == 's'){
				sendstring[0]='\0';
				sendstring = FRONT.setpoint();
				char buff[32];
				write(DATA.fd,sendstring,strlen(sendstring));
				strncpy(setpoint,sendstring+3,strlen(sendstring)-4);
		}
		else{
				write(DATA.fd,"<t_>",32);
				bytesread = read(DATA.fd,DATA.buffer,32);
				DATA.buffer[bytesread] = '\0';
				temp_str = strtok(DATA.buffer," ");
				pow_str	 = strtok(NULL," ");
				temp = atof(temp_str);
				FRONT.update(temp_str,setpoint,pow_str);
				usleep(500000);
				steps++;
	   }
    }	
}
