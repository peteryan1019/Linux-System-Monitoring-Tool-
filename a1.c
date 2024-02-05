#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <utmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_MEM_LENGTH 100  
#define MAX_SAMPLES 100 
#define LAST_IDLE_INDEX 0
#define LAST_SUM_INDEX 1
#define IDLE_INDEX 3


void print_mem_use(int currentLine, int totalLines, bool sequential_flag, char mem_usage_lines[MAX_SAMPLES][MAX_MEM_LENGTH]){
    struct sysinfo *info = malloc(sizeof(struct sysinfo));
	 if (info == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
	 }
    sysinfo(info);
	//convert bytes into gigabytes
    float total_ram = (float) info->totalram / 1000000000;
    float free_ram = (float) info->freeram / 1000000000;
    float total_swap = (float) (info->totalswap + info->totalram) / 1000000000;
    float free_swap = (float)(info->freeswap + info->freeram) / 1000000000;
	

    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
    

	//store the current information in the array
	snprintf(mem_usage_lines[currentLine - 1], MAX_MEM_LENGTH, "%.2f GB / %.2f GB-- %.2f GB/ %.2f GB \n", total_ram - free_ram, total_ram, total_swap - free_swap, total_swap);
    // Print the filled lines
	if(sequential_flag==false){
    	for (int i = 0; i < currentLine; i++) {
		printf("%s", mem_usage_lines[i]);	
	}

    // Print the empty lines for yet-to-be-filled memory usage
    	for (int i = currentLine; i < totalLines; i++) {
        printf("\n");  // Empty line
    	}
	}

	// if sequential flag is used
	else{
		for(int i=1; i<=totalLines; i++){
			if(i==currentLine){
				printf("%.2f GB / %.2f GB-- %.2f GB/ %.2f GB \n", total_ram - free_ram, total_ram, total_swap - free_swap, total_swap);
			}
			else{
				printf("\n");
			}
		}
	}

    free(info); // Free the memory after printing the usage
}



void print_user_info(){
    struct utmp current_record;  // To store information about each session
    int utmpfd;                  // File descriptor for the utmp file
    int reclen = sizeof(current_record);

    utmpfd = open(UTMP_FILE, O_RDONLY);  // Open the utmp file for reading
    if (utmpfd == -1) {
        perror(UTMP_FILE);  // Print any error message
        exit(1);
    }

    printf("### Sessions/users ###\n");
    while (read(utmpfd, &current_record, reclen) == reclen) {
        if (current_record.ut_type == USER_PROCESS) {  
            printf("%-8.8s ", current_record.ut_user); // Print the username
            printf("%-8.8s ", current_record.ut_line); // Print the tty
            printf("(%s)\n", current_record.ut_host);  // Print the host/ip
        }
    }
    close(utmpfd);  // Always close files when you're done
}

void print_header_line(){
    struct rusage *m = malloc(sizeof(struct rusage));
    getrusage(RUSAGE_SELF,m);
    printf("Memory Usage: %ld kilobytes\n",m->ru_maxrss);
	free(m);
	printf("---------------------------------------\n");
}

int * print_cpu_usage(int * cpu_time_array){
	printf("---------------------------------------\n");
    printf("Number of cores: %d\n", get_nprocs());
    char str[100];
	const char d[2] = " ";
	char* token;
	long int i, sum=0, idle;
	long double idleFraction;
		FILE* fp = fopen("/proc/stat","r");
		fgets(str,100,fp);
		fclose(fp);
		token = strtok(str,d);
				sum = 0;
		while(token!=NULL){
			token = strtok(NULL,d);
			if(token!=NULL){
				sum += atoi(token);
				}
			if(i==IDLE_INDEX){
				idle = atoi(token);
				}
			i++;
		}
			idleFraction = 100 - (float)(idle-cpu_time_array[LAST_IDLE_INDEX])*100/(sum-cpu_time_array[LAST_SUM_INDEX]);
			printf(" total cpu use: %.2Lf %%\n", idleFraction);
			cpu_time_array[LAST_IDLE_INDEX] = idle;
			cpu_time_array[LAST_SUM_INDEX] = sum;
		return cpu_time_array;
    }

void print_system_stats(){
    struct utsname *buf = (struct utsname *)malloc(sizeof(struct utsname));
    if (buf == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    
    int success = uname(buf);
    if (success == 0) {
	printf("---------------------------------------\n");
     printf("### System Information ###\n");
	 printf("System Name = %s\n", buf->sysname);
   	 printf("Machine Name = %s\n", buf->nodename);
	 printf("Version = %s\n", buf->version);
	 printf("Release = %s\n", buf->release );
	 printf("Architecture = %s\n", buf->machine);	
    }
	struct sysinfo * info = malloc(sizeof(struct sysinfo));
	int success2 = sysinfo(info);
	if(success2==0){
		int seconds = (int) info->uptime;
		int days = seconds / (24*60*60);
		int hours = (seconds % (24*3600))/3600;
		int hours1 = seconds / (60*60); 
		int minutes = ((seconds % (3600))/60);
		int seconds1 = (seconds % 60);
		printf("System running since last reboot: %d days %d:%d:%d (%d:%d:%d)\n", days, hours, minutes, seconds1,hours1 ,minutes,seconds1);
	}

    free(buf);
	free(info);
}

void perform_loop(int i, int samples, int tdelay, bool system_flag, bool user_flag, bool sequential_flag, int * cpu_time_array, char mem_usage_lines[MAX_SAMPLES][MAX_MEM_LENGTH]){

	printf("Number of samples = %d, every %d seconds\n", samples, tdelay);
	if(sequential_flag==false){
		printf("\033[H\033[J"); //clear screen and move cursor to home
		printf("Number of samples = %d, every %d seconds\n", samples, tdelay);
		print_header_line();
	}
	else{
		printf(">>>current iteration number: %d\n", i+1);
		print_header_line();
	}
	//if mode is to print everything
	if(system_flag==user_flag){
        // Print the header line for memory usage

        // Print memory usage 'i+1' times
        print_mem_use(i + 1, samples, sequential_flag, mem_usage_lines);
        // Print user information and CPU usage
        print_user_info();
        cpu_time_array=print_cpu_usage(cpu_time_array);
    	}
	//if we only have --user
	else if(user_flag==true){
			print_user_info();
	}

	//if mode is system only
	else if(system_flag==true){
		print_mem_use(i + 1, samples, sequential_flag, mem_usage_lines);
		cpu_time_array=print_cpu_usage(cpu_time_array);
	}
}

int main(int argc, char *argv[]){
    int samples = 10; // default value
    int tdelay = 1;   // default value
	int positional_arg_counter = 0;
	bool sequential_flag = false;
	bool system_flag = false;
	bool user_flag = false;

    // Process each argument provided
	for (int i = 1; i < argc; ++i) {
		if (strncmp(argv[i], "--samples=", 10) == 0) {
			if(isdigit(argv[i][10])){
			samples = atoi(argv[i] + 10);}
		} 
		
		else if (strncmp(argv[i], "--tdelay=", 9) == 0) {
			if(isdigit(argv[i][10])){
			tdelay = atoi(argv[i] + 9);}
		} 
		
		else if (isdigit(argv[i][0])) {
			if(positional_arg_counter==0){
				samples = atoi(argv[i]);
				positional_arg_counter++;
			}
			else if(positional_arg_counter==1)
			{
				tdelay = atoi(argv[i]);
			}
		} 
		else if(strncmp(argv[i], "--user", 6)==0){
			user_flag=true;
		}
		else if(strncmp(argv[i], "--system", 8)==0){
			system_flag= true;
		}
		else if (strcmp(argv[i], "--sequential") == 0) {
    		sequential_flag = true;
			}

		else {
			fprintf(stderr, "Invalid argument: %s\n, program terminated", argv[i]);
			return EXIT_FAILURE;
		}
	}
	int * cpu_time_array = malloc(2*sizeof(int));
	char mem_usage_lines[MAX_SAMPLES][MAX_MEM_LENGTH];
	for(int i=0; i<samples; ++i){
		perform_loop(i, samples, tdelay, system_flag, user_flag, sequential_flag, cpu_time_array, mem_usage_lines);
		sleep(tdelay);
	}
	free(cpu_time_array);
	print_system_stats();
    return 0;
}