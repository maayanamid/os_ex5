#include <iostream>
#include<stdio.h>
#include<sched.h>
#include<unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/wait.h>
#include<sys/mount.h>
#include <fstream>
#include <signal.h>

#define STACK 8192
#define SYS_ERROR "system error: "
#define MEM_ERROR "memory allocation failed"
#define CLONE_ERROR "clone failure"
#define UMOUNT_ERROR "umount failure"
#define REMOVE_ERROR "remove failure"
#define WAIT_ERROR "wait failure"
#define HOST_ERROR "hostname failure"
#define CHROOT_ERROR "chroot failure"
#define MKDIR_ERROR "mkdir failure"
#define CHDIR_ERROR "chdir failure "
#define MOUNT_ERROR "mount failure"
#define EXECVP_ERROR "execvp failure"
#define FILE_ERROR "file failure"
#define ARG_NUM_ERROR "not enough arguments given"
#define RMDIR_ERROR "rmdir failed"
#define SUCCESS 0
#define EXIT_FAIL 1
#define FAILURE_CODE -1

int container(void* arg) {
	char** argv = (char **)arg;
	char* new_hostname = argv[1];
	char* new_filesystem_dir = argv[2];
	char* num_processes = argv[3];
	char* path_to_program_to_run_within_container = argv[4];
	char** args_for_program = argv + 5; //TODO fix argument passing

	// change hostname
	if (sethostname(new_hostname, strlen(new_hostname)) == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << HOST_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}

    //change working directory
    if (chdir(new_filesystem_dir) == FAILURE_CODE) {
        std::cerr << SYS_ERROR << CHDIR_ERROR << strerror(errno) << std::endl;
        exit(EXIT_FAIL);
    }

	// change root
	if (chroot(new_filesystem_dir) == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << CHROOT_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}

    std::string dirs[] = {"sys/fs", "sys/fs/cgroups", "sys/fs/cgroups/pids"};
	for (std::string s : dirs) {
        if (mkdir(s.c_str(), 0755) == FAILURE_CODE) {
            std::cerr << SYS_ERROR << MKDIR_ERROR << strerror(errno) << std::endl;
            exit(EXIT_FAIL);
        }
    }

	// To attach the container process into this new cgroup, you need to write the process’s pid into the file “cgroup.procs” under the new directory we created
	std::ofstream cgroupProcs("sys/fs/cgroups/pids/cgroup.procs");
	if (cgroupProcs.bad()) {
	    std::cerr << SYS_ERROR << FILE_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	cgroupProcs << "1";
	cgroupProcs.close();

	// Next, we need to write into the file “pids.max” the number of processes which is allowed
	std::ofstream pidsMax("sys/fs/cgroups/pids/pids.max");
	if (pidsMax.bad()) {
	    std::cerr << SYS_ERROR << FILE_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	pidsMax << num_processes;
	pidsMax.close();


	// Finally, to release the resources when the container is finished, we will write into the file “notify_on_release” the value 1
	std::ofstream notifyOnRelease("sys/fs/cgroups/pids/notify_on_release");
	if (notifyOnRelease.bad()) {
	    std::cerr << SYS_ERROR << FILE_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	notifyOnRelease << "1";
	notifyOnRelease.close();

	//Mount
	int c = mount("proc", "/proc", "proc", 0, 0);
	if (c != SUCCESS) {
	    std::cerr << SYS_ERROR << MOUNT_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}

	//find args TODO
//	for (int i = 5; i < argc; i++){
//	    std::cerr << arg[i] << std::endl;
//	}
	//  run the terminal/new program
	int ret = execvp(path_to_program_to_run_within_container, args_for_program);
    if (ret == FAILURE_CODE) {
        std::cerr << SYS_ERROR << EXECVP_ERROR << strerror(errno) << std::endl;
        exit(EXIT_FAIL);
    }
	return 0;
}


int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << SYS_ERROR << ARG_NUM_ERROR << std::endl;
        exit(EXIT_FAIL);
    }
	char* stack = (char*) malloc(STACK);
	if (stack == nullptr) {
	    std::cerr << SYS_ERROR << MEM_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	std::string new_filesystem_dir = argv[2];
	int container_pid = clone(container, stack + STACK, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, argv);
	if (container_pid == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << CLONE_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	if (wait(nullptr) == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << WAIT_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	if (umount((new_filesystem_dir + "/proc").c_str()) == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << UMOUNT_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}

	//remove files
	std::string files[] = {"/sys/fs/cgroups/pids/cgroup.procs", "/sys/fs/cgroups/pids/pids.max", "/sys/fs/cgroups/pids/notify_on_release"};
    for (std::string s : files) {
        if (remove((new_filesystem_dir + s).c_str()) != SUCCESS) {
            std::cerr << SYS_ERROR << REMOVE_ERROR << strerror(errno) << std::endl;
            exit(EXIT_FAIL);
        }
    }

    //remove dirs
    std::string dirs[] = {"/sys/fs/cgroups/pids", "/sys/fs/cgroups", "/sys/fs"};
    for (std::string s : dirs) {
        if (rmdir((new_filesystem_dir + s).c_str()) == FAILURE_CODE) {
            std::cerr << SYS_ERROR << RMDIR_ERROR << std::endl;
            exit(EXIT_FAIL);
        }
    }

	free(stack);
}





