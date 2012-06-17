/*            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                    Version 2, December 2004
 *
 * Copyright (C) 2012 Paul Grandperrin <paul.grandperrin@gmail.com>
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 */



/**
 * We need some feature test macros enabled to activate
 * the SA_RESTART sigaction flag.
 *
 * This flag makes the kernel automatically restart some system calls
 * across signals.
 *
 * Without this flags, in our case, the syscall accept, which is
 * usually blocked in the main process, would return with an
 * error EINTR (Interrupted system call) each time a child die
 * because of the signal interruption.
 *
 * Anyway, our code will still compile and work if this flag is not supported.
 * If the accept syscall return with error EINTR, the code will just
 * restart from the beginning of the accept loop without any futher
 * aftermath.
 *
 * See SIGACTION(2), FEATURE_TEST_MACROS(7) and SIGNAL(7) at section
 * "Interruption of System Calls and Library Functions by Signal Handlers"
 */

#include "defobj.h"
//#define _POSIX_SOURCE
//#define _XOPEN_SOURCE 500
//#define _BSD_SOURCE
#define _GNU_SOURCE /* includes most non-conflicting standards */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> /* memset */
#include <signal.h>
#include <errno.h>
#include <netdb.h> /* getnameinfo & getaddrinfo */
#include <fcntl.h> /* open flags */

#ifdef __linux__
#include <sys/prctl.h> /* prctl to activate SECCOMP sandboxing */
#endif

#include <sys/socket.h>
#include <sys/types.h> /* historical BSD compatibility */
#include <sys/wait.h> /* waitpid */
#include <sys/stat.h> /* umask open */

/* include our architecture independant object structure */
#include "defobj.h"

/**
 * Function launched for each client in a
 * new process with a fork().
 */
void handleClient(int sockCli, int sandbox_flag);

/**
 * Set up the SIGCHLD signal handler.
 */
void register_sigchld_handler();

/**
 * Signal handler which gracefully kills
 * client processes
 */
void sigchld (int sig);

/**
 * Function to try to get root privilege if possible.
 */
void try_get_privilege();

/**
 * Function to drop root privilege if any.
 */
void drop_all_privilege();

/**
 * Function to fork off in background.
 */
void daemonize();

/**
 * Chroot the process in an empty temporary directory.
 */
void chrootme();

/**
 * Sandbox the process using SECCOMP
 */
void sandboxme();

void print_usage()
{
	fprintf(stderr,
"Error: Need at least one argument.\n\
Usage: server service/port [-d] [-c] [-s]\n\
\t-d   daemonize\n\
\t-c   chroot\n\
\t-s   use sandboxing with SECCOMP\n");
}

int main ( int argc, char* argv[] ) {

	int daemonize_flag=0;
	int chroot_flag=0;
	int sandbox_flag=0;
	char* protocol;

	if (argc < 2) {
		print_usage();
		goto exit_failure;
	}
	else
		protocol=argv[1];

	int opt;
	while ((opt = getopt(argc, argv, "cds")) != -1) {
		switch (opt) {
			case 'd':
				daemonize_flag = 1;
				break;
			case 'c':
				chroot_flag = 1;
				break;
			case 's':
				sandbox_flag = 1;
				break;
			default:
				print_usage();
				goto exit_failure;
		}
	}

	/* server and client sockets */
	int sockServ, sockCli;

	/* error code returned by function calls */
	int err;

	/**
	 * Open TCP/IPv6 socket
	 */
	struct addrinfo hints;

	memset(&hints, 0, sizeof hints); /* bzero is deprecated */

	/** we specify our socket contraints */
	hints.ai_family = AF_INET6; /* AF_UNSPEC : either IPv6 or IPv6 if available */
	hints.ai_socktype = SOCK_STREAM; /* in this context means TCP */
	hints.ai_flags = AI_PASSIVE; /* use my IP address */

	/**
	 * Get a linked list of socket configurations which
	 * satisfy our constraints.
	 */
	struct addrinfo *result, *rp;
	err=getaddrinfo(NULL,protocol, &hints, &result);
	if(err) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		goto exit_failure;
	}

	/**
	 * To bind our socket, we might need to have root privileges if
	 * port <= 1024.
	 * We try to get root privilege in case our executable has the
	 * suid flag on.
	 */
	try_get_privilege();

	/** We test each socket configuration until one succeed */
	for(rp=result; rp!=NULL;rp=rp->ai_next) {

		sockServ=socket ( rp->ai_family,rp->ai_socktype,rp->ai_protocol );
		if ( sockServ==-1 ) {
			perror("socket");
			continue;
		}


		/**
		 * Configure socket to reuse by force the listenning port
		 * even if not already closed.
		 * NOTE this is the behavior of most Unix daemons.
		 */
		int on=1,off=0;
		err = setsockopt(sockServ,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
		if(err) perror("setsockopt with SO_REUSEADDR");

		/**
		* We don't want to bind our socket to only IPv6 but also IPv4.
		* Binding to both is the default on most OS with the notable
		* exception of Windows and some BSDs.
		* We turn it off just to be sure.
		*/
		err = setsockopt(sockServ,IPPROTO_IPV6,IPV6_V6ONLY,&off,sizeof(off));
		if(err) perror("setsockopt with IPV6_V6ONLY");


		err=bind(sockServ, rp->ai_addr, rp->ai_addrlen);
		if(err) {
			perror("bind");
			close(sockServ);
			continue;
		}

		/* we binded successfully */
		break;
	}
	/* free the result linked list allocated by the
	 * getaddrinfo function call.
	 */
	freeaddrinfo(result);

	/* we didn't connected successfully */
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		goto exit_failure;
	}

	/**
	 * We listen to the maximum possible connection at the same time
	 * NOTE this is the default behavior of most Unix daemons.
	 */
	err=listen ( sockServ, SOMAXCONN);
	if(err){perror("listen");goto close_socket;}

	/**
	 * If we've been asked to fork of in the background, do it
	 */
	if(daemonize_flag)
		daemonize();

	if(chroot_flag)
		chrootme();

	/* now that we have bound and chrooted, we can drop all privileges */
	drop_all_privilege();

	/**
	 * Register the SIGCHLD signal handler.
	 */
	register_sigchld_handler();

	/**
	 * This is the main loop which accept new connections
	 * and dispatch them in new processes using fork().
	 */
	for (;;) {
		sockCli=accept ( sockServ,NULL, NULL);
		if(sockCli<=0) {
			/**
			 * This error code means that the system call was interrupted
			 * by a signal handler. To avoid this error, use, if available,
			 * the SA_RESTART flag when calling sigaction.
			 *
			 * We know that this isn't an error an therefor continue to
			 * the beginning of the loop.
			 */
			if(errno == EINTR)
				continue;

			perror("accept");
			break;
		}

		/**
		 * We fork
		 */
		pid_t pid=fork();
		if(pid < 0) perror("fork");

		if(pid==0) { /** Child's path */
			close(sockServ); //Probably useless?

			handleClient(sockCli,sandbox_flag);
			close(sockCli);

			exit(EXIT_SUCCESS);
		}

		/**
		* Father's path,
		* we loop again on accepting connections
		*/
	}

	close(sockServ);
	return EXIT_SUCCESS;

	close_socket:
	close ( sockServ );

	exit_failure:
	return EXIT_FAILURE;
}

void handleClient(int sockCli, int sandbox_flag)
{
	char protobuf[255];
	char namebuf[255];
	/* size could have been INET6_ADDRSTRLEN if we
	 * wanted to display only IPv6 numeric value
	 */

	struct sockaddr_storage ss;
	socklen_t ss_len=sizeof(ss);

	/**
	 * We get and display client socket informations.
	 */
	int err;
	err=getpeername(sockCli,(struct sockaddr*)&ss,&ss_len);
	if(err)
		perror("getnameinfo");
	else {
		err=getnameinfo((struct sockaddr *)&ss, ss_len,
			namebuf, sizeof(namebuf), protobuf, sizeof(protobuf), 0);

		if(err)
			perror("getnameinfo");
		else {
			printf("Client name/address is %s\n", namebuf);
			printf("Client protocol/port is %s\n", protobuf);
		}
	}


	/**
	 * FIXME
	 * RANDOM SEND AND RECEIVE STUFF FOR THE MOMENT
	 */

	int rcdsize=sizeof(obj);
	err=setsockopt(sockCli, SOL_SOCKET, SO_RCVLOWAT, &rcdsize,sizeof(rcdsize));
	if(err) {
		perror("setsockopt(SO_RCVLOWAT) failed");
		return;
	}

	if(sandbox_flag)
		sandboxme();

	/**
	 * From here we can't use recv and send anymore because of
	 * the potential sandboxing restricting use to use read, write,
	 * _exit and sigreturn syscalls.
	 * See PRCTL(2) for more informations.
	 */

	obj buf;
	int nb_obj;

	if(read(sockCli,&buf, rcdsize)!=rcdsize)
		return;

	nb_obj=ntohl(buf.ii);

	do {

		if(read(sockCli,&buf, rcdsize)!=rcdsize)
			return;

		//We convert from network endianness and then cast without implicit binary convertion
		union double2uint64 tmp;
		tmp.u=htonll(buf.dd);
		printf("\"%s\", \"%s\", %d, %d, %f, %d\n",buf.a,buf.b,ntohl(buf.ii),ntohl(buf.jj),tmp.d,buf.iqt);

		nb_obj--;
	} while(buf.iqt!=-1 && nb_obj != 0);

	err=write(sockCli,"a",1);
	if(err < 0)
		perror("write");

	printf("Bye\n\n");
}


/**
 * Set up the SIGCHLD signal handler.
 */
void register_sigchld_handler()
{

	struct sigaction act;

	sigemptyset(&act.sa_mask);
	act.sa_flags=0;

	/**
	 * The SA_NOCLDSTOP flag masks signals about children being stopped
	 * or resumed.
	 */
	#ifdef SA_NOCLDSTOP
	act.sa_flags |= SA_NOCLDSTOP;
	#endif

	/**
	 * The SA_RESTART flag make some syscall restartable after being paused
	 * being paused by signal handling.
	 */
	#ifdef SA_RESTART
	act.sa_flags |= SA_RESTART; // must use X/OPEN or UNIX98 standard to use it
	#endif

	//act.sa_flags|=SA_NOCLDWAIT; // no zombie please

	act.sa_handler = sigchld;

	if (sigaction(SIGCHLD, &act, NULL)) {
		perror ("sigaction");
		return;
	}
}

/**
 * The SIGCHLD handler.
 */
void sigchld (int sig)
{
	(void) sig;

	/**
	 * Wait for all dead processes.
	 * We use a non-blocking call to be sure this signal handler will not
	 * block if a child was cleaned up in another part of the program.
	 */
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

void try_get_privilege()
{
	int err=0;
	uid_t ruid,euid;

	ruid=getuid();
	euid=geteuid();

	/* If we're not root and we can be root */
	if(ruid != 0 && euid == 0)
	{
		printf("getting root privileges\n");
		err=setuid(0); /* effectively get root privilege */
		if(err) perror("seteuid");
	}
}


void drop_all_privilege()
{
	int err=0;

	/* We must be root first to drop privileges */
	try_get_privilege();

	if(getuid() == 0)
	{
		/**
		 * We drop all privileges we can by dropping to daemon(uid=1,gid=1) privileges.
		 * This way, there will be no way back.
		 */

		printf("dropping root privileges\n");

		err |= setgid(1);
		err |= setegid(1);
		err |= setuid(1);
		err |= seteuid(1);
		if(err) fprintf(stderr,"drop privilege failed");
	}

}

void daemonize()
{
	printf("forking off in background\n");
	pid_t pid,sid;
	int err;

	pid = fork();
	if (pid < 0) {
		perror("fork");
		return;
	}

	if(pid > 0) /* Father path */
	{
		_exit(EXIT_SUCCESS);
	}

	/* Children path */

	umask(0);

	/**
	 * Put the process in a new session ID and
	 * detach it from the controlling tty.
	 */
	sid = setsid();
	if(sid < 0) {
		perror("setsid");
		return;
	}

	/* close all descriptors */
	//for (int i=sysconf(_SC_OPEN_MAX);i>=0;--i) close(i);

	close(0);
	close(1);
	close(2);
	/* TODO check return value */
	err=open("/dev/null",O_RDWR); /* open stdin */
	err|=dup(0); /* stdout */
	err|=dup(0); /* stderr */

	if(err)
		perror("open/dup");
}

void chrootme()
{
	int err;
	/**
	 * 	TODO correctly deal with return values
	 */

	/**
	 * We must be root to use the chroot syscall.
	 */
	try_get_privilege();
	if(getuid() != 0)
		return;

	char* tmpdir=getenv("TMPDIR");
	if(tmpdir == NULL)
		tmpdir="/tmp";

	err = chdir(tmpdir);
	if(err) {
		perror("chdir");
		return;
	}

	char tmpchroot[]="server-XXXXXX";
	char *strerr;
	strerr=mkdtemp(tmpchroot);
	if(strerr == NULL) {
		perror("mkdtemp");
		return;
	}

	printf("chrooting in %s/%s\n",tmpdir,tmpchroot);
	err =  chroot(tmpchroot);
	if(err) {
		perror("chroot");
		return;
	}

	err = chdir("/");
	if(err) {
		perror("chdir");
		return;
	}
}

void sandboxme()
{
#if defined __linux__ &&  PR_SET_SECCOMP
	int err;

	err=prctl(PR_SET_SECCOMP,1);
	if (err < 0)
		perror("prctl(PR_SET_SECCOMP,1)");
#endif
}

