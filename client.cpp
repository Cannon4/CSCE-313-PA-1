/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Cannon Cavazos
	UIN: 434000356
	Date: 9/18/25
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <chrono>
using namespace std::chrono;


using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int c = 0;
	int mes_len = MAX_MESSAGE;
	


	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				mes_len = atoi (optarg);
				break;
			case 'c':
				c = 1;
				break;
			
		}
	}
	pid_t pid = fork();

    	if (pid < 0) {
        	perror("fork");
        	return 1;
    	}

    	if (pid == 0) {
        	execl("./server", "./server", (char*)NULL);
        	perror("execl"); // only runs if execl fails
        	exit(1);
    	}

    	FIFORequestChannel* control_chan = new FIFORequestChannel ("control", FIFORequestChannel::CLIENT_SIDE);
	FIFORequestChannel* chan = control_chan;
	FIFORequestChannel* chan1 = nullptr;
	if(c == 1){
		MESSAGE_TYPE m = NEWCHANNEL_MSG;
		chan->cwrite(&m, sizeof(MESSAGE_TYPE));
		char channel_buf[MAX_MESSAGE];
		chan->cread(channel_buf, sizeof(channel_buf));
		string new_channel(channel_buf);
		chan1 = new FIFORequestChannel (new_channel, FIFORequestChannel::CLIENT_SIDE);
		chan = chan1;
	}

	// example data point request
    	char buf[MAX_MESSAGE]; // 256
	if(p != -1 && t != -1 && e != -1){
		datamsg x(p, t, e);
		memcpy(buf, &x, sizeof(datamsg));
		chan->cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan->cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	else if(p != -1 && t == -1 && e == -1){
		std::ofstream file("received/x1.csv");
		cout << "Writing to x1.csv" << endl;
		for(int i = 0; i < 1000; i++){
			//Gets ecg1 and writes to file
			file<<i*0.004 << ",";
			datamsg x(p, i*0.004, 1);
			memcpy(buf, &x, sizeof(datamsg));
			chan->cwrite(buf, sizeof(datamsg)); // question
			double reply1;
			chan->cread(&reply1, sizeof(double)); //answer
			file<<reply1<<",";

			//Gets ecg2 and writes to file
			datamsg y(p, i*0.004, 2);
			memcpy(buf, &y, sizeof(datamsg));
			chan->cwrite(buf, sizeof(datamsg)); // question
			double reply2;
			chan->cread(&reply2, sizeof(double)); //answer
			file<<reply2<<"\n";
		}
		file.close();
	}
	
	
	
    // sending a non-sense message, you need to change this
	auto start = high_resolution_clock::now();
	if(!filename.empty()){
		filemsg fm(0, 0);
		string fname = filename;
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan->cwrite(buf2, len);  // I want the file length;
		__int64_t file_len;
		chan->cread(&file_len, sizeof(__int64_t));
		cout << file_len << endl;
		delete[] buf2;
		int offset  = 0;
		ofstream file("received/" + filename, ios::binary);
		while(offset < file_len){
			int curr_chunk = min(mes_len, static_cast<int>(file_len - offset));
			filemsg fm1(offset, curr_chunk);
			int len = sizeof(filemsg) + (fname.size() + 1);
			char* buf2 = new char[len];
			memcpy(buf2, &fm1, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), fname.c_str());
			chan->cwrite(buf2, len);
			char* data = new char[curr_chunk];
			chan->cread(data, curr_chunk);
			file.write(data, curr_chunk);
			delete[] buf2;
			delete[] data;
			offset = offset + curr_chunk;
		}
		file.close();
	}
	auto end = high_resolution_clock::now();
	double seconds = duration_cast<duration<double>>(end - start).count();
	cout << "Transfer took " << seconds << " seconds\n";

	

	
	
	// closing the channel
    	MESSAGE_TYPE m = QUIT_MSG;
    	chan->cwrite(&m, sizeof(MESSAGE_TYPE));
	if(chan1 != nullptr){
		delete chan1;
	}
	control_chan->cwrite(&m, sizeof(MESSAGE_TYPE));
	delete control_chan;
}
