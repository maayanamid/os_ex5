#include <iostream>
#include<stdio.h>
#include<sched.h>
#include<unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <fstream>

#define STACK 8192

int container(void* arg) {
	char** argv = (char **)arg;
	char* new_hostname = argv[0];
	char* new_filesystem_dir = argv[1];
	char* num_processes = argv[2];
	char* path_to_program_to_run_within_container = argv[3];
	char** args_for_program = argv + 4;

	// mount filesystem

	// change root
	chroot(new_filesystem_dir);
	mount("proc", "/proc", "proc", 0, 0);
	mkdir("/sys/fs/cgroup/pids");

	// To attach the container process into this new cgroup, you need to write the process’s pid into the file “cgroup.procs” under the new directory we created
	ofstream cgroupProcs("/sys/fs/cgroup/pids/cgroup.procs");
	cgroupProcs << "1";
	cgroupProcs.close();

	// Next, we need to write into the file “pids.max” the number of processes which is allowed
	ofstream pidsMax("/sys/fs/cgroup/pids/pids.max");
	pidsMax << num_processes;
	pidsMax.close();

	// Finally, to release the resources when the container is finished, we will write into the file “notify_on_release” the value 1
	ofstream pidsMax("/sys/fs/cgroup/pids/notify_on_release");
	pidsMax << "1";
	pidsMax.close();

	// change hostname
	sethostname(new_hostname, strlen(new_hostname));
	
	//  run the terminal/new program
	int ret = execvp(path_to_program_to_run_within_container, args_for_program);

	return 0;
}


int main(int argc, char* argv[]) {
	void* stack = malloc(STACK);
	int container_pid = clone(container, stack + STACK, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, argv);
	wait(NULL);
	umount(); // TODO see if require arguments
	free(stack);
}





