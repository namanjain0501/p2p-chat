#include <bits/stdc++.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <unistd.h> 
#include <arpa/inet.h>

using namespace std;

#define RED cout << "\033[1;31m"
#define LIGHT_GREEN cout << "\033[2;32m"
#define GREEN cout << "\033[1;32m"
#define YELLOW cout << "\033[1;33m"
#define RESET cout<<"\033[0m"

// timeout interval in seconds
#define TIMEOUT_INV 30

struct info{
	int port;
	string ip;
	string name;

	info(){}

	info(int a, string b, string c):
		port(a), ip(b), name(c){}
};

vector<info>info_table;

int get_port_from_name(string name)
{
	for(int i=0;i<info_table.size();i++)
	{
		if(info_table[i].name==name)
			return info_table[i].port;
	}
	// return -1 if not found
	return -1;
}

string get_ip_from_name(string name)
{
	for(int i=0;i<info_table.size();i++)
	{
		if(info_table[i].name==name)
			return info_table[i].ip;
	}
	// return -1 if not found
	return "";
}

string get_name_from_port(int port)
{
	for(int i=0;i<info_table.size();i++)
	{
		if(info_table[i].port==port)
			return info_table[i].name;
	}
	// return empty string if not found
	return "";	
}

int main()
{
	info if1(12000,"127.0.0.1","alice");
	info if2(12010,"127.0.0.1","bob");
	info if3(12020,"127.0.0.1","dave");
	info if4(12030,"127.0.0.1","charlie");
	info if5(12040,"127.0.0.1","frank");
	info_table.push_back(if1);
	info_table.push_back(if2);
	info_table.push_back(if3);
	info_table.push_back(if4);
	info_table.push_back(if5);

	string username;

	while(1)
	{
		YELLOW;
		cout<<"Please Enter Your Name (alice/bob/dave/charlie/frank) : ";
		RESET;
		cin>>username;

		// to consume \n after name
		string dump;
		getline(cin,dump);

		if(get_port_from_name(username)==-1)
		{
			RED;
			cout<<"Name Invalid! Please enter again!!"<<endl;
			RESET;
		}
		else
			break;
	}

	int server_fd, client_fd;
	struct timeval timeout;

	// creating server socket
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed"); 
		exit(-1);
	}


	// setting up address and port
	sockaddr_in serv_address, cli_address, peer_addr; 
	serv_address.sin_family = AF_INET;

	char *buf;
	buf = &(get_ip_from_name(username)[0]);
	serv_address.sin_addr.s_addr = inet_addr(buf);
	serv_address.sin_port = htons(get_port_from_name(username));

	int opt = 1;

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
    }

	// binding address to server socket
	if(bind(server_fd, (struct sockaddr *)&serv_address, sizeof(serv_address)) < 0)
	{
		perror("bind failed");
		exit(-1);
	}

	// listening for requests
	listen(server_fd,4);

	int maxfd;

	fd_set read_set;
	maxfd=server_fd+1;

	// emptying read_set
	FD_ZERO(&read_set);
	// inserting server file descriptor in read_set
	FD_SET(server_fd, &read_set);
	// inserting stdin in read_set
	FD_SET(0,&read_set);

	// used to store connection fds of peers
	map<string,int>peer_fds;

	// when was the last time was this file descriptor was used
	map<int,time_t>last_used;

	while(1)
	{
		// emptying read_set
		FD_ZERO(&read_set);
		// inserting server file descriptor in read_set
		FD_SET(server_fd, &read_set);
		// inserting stdin in read_set
		FD_SET(0,&read_set);
		maxfd=server_fd+1;

		for(auto &it:peer_fds)
		{
			if(it.second!=-1)
			{
				FD_SET(it.second, &read_set);
				maxfd=max(maxfd, it.second+1);
			}
		}

		timeout.tv_sec=TIMEOUT_INV;
		// calling select system call
		int result = select(maxfd, &read_set, NULL, NULL, &timeout);

		// check if some msg came
		for(auto &it:peer_fds)
		{
			if(it.second==-1)
				continue;
			if(FD_ISSET(it.second, &read_set))
			{
				char buf[500];
				int sz = read(it.second, buf, 500);
				// connection is closed
				if(sz==0)
				{
					last_used[it.second]=-1;
					close(it.second);
					peer_fds[it.first]=-1;
					continue;
				}
				GREEN;
				cout<<"Message from "<<it.first;
				RESET;
				cout<<": "<<buf<<endl;

				last_used[it.second]=time(NULL);
			}
			else if( last_used[it.second]!=-1 && (float)(time(NULL) - last_used[it.second]) >= TIMEOUT_INV )
			{
				close(it.second);	

				// closing fd for no response
				last_used[it.second]=-1;
				peer_fds[it.first]=-1;
			}
		}

		// new connection request
		if(FD_ISSET(server_fd, &read_set))
		{
			socklen_t len = sizeof(cli_address);
			int new_fd = accept(server_fd, (struct sockaddr *)&cli_address,&len);


			char ip[INET_ADDRSTRLEN];
        	inet_ntop(AF_INET, &(cli_address.sin_addr), ip, INET_ADDRSTRLEN);

			string friendname = get_name_from_port((int)ntohs(cli_address.sin_port));
			if(friendname == "")
			{
				RED;
				cout<<"invalid request"<<endl;
				RESET;
			}
			else
			{
				peer_fds[friendname]=new_fd;
				last_used[new_fd]=time(NULL);
			}
		}

		// some input from user came
		if(FD_ISSET(0, &read_set))
		{
			// read msg 
			string input;
			getline(cin,input);

			int is_msg=0;
			string friendname="", msg="";
			// separating msg and friendname
			for(int i=0;i<input.size();i++)
			{
				if(!is_msg)
				{
					if(input[i]=='/')
						is_msg=1;
					else
						friendname+=input[i];
				}
				else
					msg+=input[i];
			}

			// if friendname is valid
			if(get_port_from_name(friendname)!=-1)
			{
				int flag=0;// to chk if msg has been sent

				// if socket already exists
				if(peer_fds.count(friendname) && peer_fds[friendname]!=-1)
				{
					char *buf;
					buf = &msg[0];
					if(send(peer_fds[friendname], (char *)buf, sizeof(msg), 0) >= 0)
					{
						LIGHT_GREEN;
						cout<<"Message sent successfully to "<<friendname<<"!";
						RESET;
						cout<<endl;

						last_used[peer_fds[friendname]]=time(NULL);
						flag=1;
					}
					
				}
				else if(!flag)
				{
					// creating new socket
					client_fd = socket(AF_INET, SOCK_STREAM, 0);

					cli_address.sin_family = AF_INET;
					char *buf;
					buf = &(get_ip_from_name(friendname)[0]);
					cli_address.sin_addr.s_addr = inet_addr(buf);
					cli_address.sin_port = htons(get_port_from_name(friendname));

					int opt = 1;

					if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
				    {
				    	RED;
				        perror("setsockopt");
				        RESET;
				    }
					

					if(bind(client_fd, (struct sockaddr *)&serv_address, sizeof(serv_address)) < 0)
					{
						RED;
						perror("bind failed");
						RESET;
					}

					if(connect(client_fd, (struct sockaddr *)&cli_address, sizeof(cli_address)) < 0)
					{
						RED;
						cout<<"Connection to "<<friendname<<" cannot be made!";
						RESET;
						cout<<endl;
						last_used[client_fd]=-1;
						close(client_fd);
					}
					else
					{
						peer_fds[friendname]=client_fd;

						// cout<<"fd created: "<<client_fd<<endl;

						buf = &msg[0];
						send(client_fd, (char *)buf, sizeof(msg), 0);
						LIGHT_GREEN;
						cout<<"Message sent successfully to "<<friendname<<"!";
						RESET;
						cout<<endl;

						last_used[client_fd]=time(NULL);
					}
				}
			}
			else
			{
				RED;
				cout<<"Invalid Friend name!! Try friendname/<msg>";
				RESET;
				cout<<endl;
			}
		}	
	}

	return 0;
}