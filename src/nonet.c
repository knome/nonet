
// 
// nonet local-command [ arguments ... ]
//   
//   runs the command in a private net without networking
//   

#define _GNU_SOURCE

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <linux/wait.h>

#define FAIL_INVOCATION_ERROR 125
#define FAIL_CANNOT_EXECUTE   126
#define FAIL_NOT_FOUND        127
#define FAIL_LO_UP_ERROR      128

int check_and_pop_lo_arg( int *, char *** );
void run_net_lo_up( void );
void wait_for_net_lo( int );
void run_net_lo( void );

int
main(
  int     argc ,
  char ** argv
){
  // we expect to have setuid/setgid permission bits on the executable
  // so we unshare the network to create our captured space
  // then we set our effective group and user ids to the callers actual ones
  
  if( argc < 2 ){
    fprintf( stderr, "nonet error : no command given\n" );
    return FAIL_INVOCATION_ERROR ;
  }
  
  if( unshare( CLONE_NEWNET ) == -1 ){
    fprintf( stderr, "nonet error : could not unshare net namespace : %d : %s\n", errno, strerror( errno ) );
    return FAIL_INVOCATION_ERROR ;
  }
  
  int uid = getuid();
  
  if( check_and_pop_lo_arg( &argc, &argv ) ){
      run_net_lo_up();
  }
  
  // drop permissions
  
  if( setegid( getgid() ) == -1 ){
    fprintf( stderr, "failed to setegid to getgid, %s", strerror( errno ) );
    return FAIL_INVOCATION_ERROR ;
  }
  
  if( setuid( uid ) == -1 ){
    fprintf( stderr, "failed to setuid to initial uid, %s", strerror( errno ) );
    return FAIL_INVOCATION_ERROR ;
  }
  
  if( seteuid( uid ) == -1 ){
    fprintf( stderr, "failed to seteuid to initial uid, %s", strerror( errno ) );
    return FAIL_INVOCATION_ERROR ;
  }
  
  char ** command = &argv[1] ;
  
  if( execvp( *command, command ) == -1 ){
    int ee = errno ;
    fprintf( stderr, "nonet error : could not execute given command : %d : %s\n", ee, strerror( ee ) );
    if( ee == ENOENT ){
      return FAIL_NOT_FOUND ;
    } else {
      return FAIL_CANNOT_EXECUTE ;
    }
  }
  
  return 0 ;
}

int
check_and_pop_lo_arg(
  int *    pargc ,
  char *** pargv
){
  char ** argv = * pargv ;
  
  // check that arg 0 ( program name ) is set before stepping to arg 1
  if( ! argv[0] ){
    return 0 ;
  }
  
  // if there is no arg 1, it can't be --lo
  if( ! argv[1] ){
    return 0 ;
  }
  
  // determine if arg 1 is not --lo
  // if it's not --lo but starts with -, fail with an unknown parameter issue
  if( strcmp( argv[1], "--lo" ) != 0 ){
    if( argv[1][0] == '-' ){
      fprintf( stderr, "nonet error: unknown option: %s\n", argv[1] );
      exit( FAIL_INVOCATION_ERROR );
    }
    
    // if it doesn't start with -, it's just the program to run, just return that there's no --lo
    return 0;
  }
  
  // make sure there are no more arguments 
  if( argv[2] ){
    if( argv[2][0] == '-' ){
      fprintf( stderr, "nonet error: unexpected second option: %s\n", argv[2] );
      exit( FAIL_INVOCATION_ERROR );
    }
  }
  
  // there is a --lo, "pop" it before returning so
  
  (*pargc)-- ;
  (*pargv)++ ;
  
  return 1 ;
}

void
run_net_lo_up(
  void
){
  int rv = fork();
  
  if( rv < 0 ){
    fprintf( stderr, "failed to invoke '/sbin/ip link set dev lo up': %s\n", strerror( errno ) );
    exit( FAIL_LO_UP_ERROR );
  }
  
  if( rv == 0 ){
    // child process
    run_net_lo();
  }
  
  if( rv > 0 ){
    // parent process
    wait_for_net_lo( rv );
  }
  
  return ;
}

void
wait_for_net_lo(
  int pid
){
 retry:;
  int status = 0 ;
  int rv = waitpid( pid, &status, 0 );
  if( rv < 0 ){
    if( errno == EINTR ){
      goto retry;
    }
    
    fprintf( stderr, "nonet error: lost --lo fork child?: %s\n", strerror( errno ) );
    exit( FAIL_LO_UP_ERROR );
  }
  
  if( rv != pid ){
    fprintf( stderr, "nonet error: impossible error, got wrong child?\n" );
    exit( FAIL_LO_UP_ERROR );
  }
  
  if( ! WIFEXITED( status ) ){
    // we got some other wait signal, even possible?
    goto retry;
  }
  
  if( WEXITSTATUS( status ) != 0 ){
    // ip command failed
    fprintf( stderr, "nonet error: ip command failed\n" );
    exit( FAIL_LO_UP_ERROR );
  }
  
  // it ran successfully, everything is good
  return ;
}

void
run_net_lo( void ){
  // this is run in a fork
  // exec the ip command or exit with failure
  
  // be root to run ip link set dev lo up as root
  // setuid( 0 );
  
  // use an absolute path to ip to avoid hijacking
  char * program = "/sbin/ip" ;
  
  // pass program-name + arguments to ip
  char * programArgs[] = {
    "ip",
    "link",
    "set",
    "dev",
    "lo",
    "up",
    NULL
  };
  
  // don't pass any envvars to ip
  char * programEnv[] = {
    NULL
  };
  
  // don't use system or a "p" variant of exec that does PATH lookups to avoid hijacking
  
  int rv = execve( program, programArgs, programEnv );
  
  if( rv == -1 ){
    fprintf(
      stderr ,
      "nonet error : error launching or waiting on child process while bringin up lo\n"
    );
    
    exit( FAIL_LO_UP_ERROR );
  }
}  
