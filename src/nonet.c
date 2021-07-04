
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

#define FAIL_INVOCATION_ERROR 125
#define FAIL_CANNOT_EXECUTE   126
#define FAIL_NOT_FOUND        127
#define FAIL_LO_UP_ERROR      128

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
  
  if( argc > 1 ){
    if(
      argv[0] &&
      argv[1] &&
      argv[1][0] == '-' &&
      argv[1][1] == '-' &&
      argv[1][2] == 'l' &&
      argv[1][3] == 'o'
    ){
      // be root to run ip link set dev lo up as root
      setuid( 0 );
      
      int rv = system( "ip link set dev lo up" );
      if( rv == -1 ){
        fprintf(
          stderr ,
          "nonet error : error launching or waiting on child process while bringin up lo\n"
        );
        
        return FAIL_LO_UP_ERROR ;
      }
      
      int es = WEXITSTATUS( rv );
      
      if( es != 0 ){
        fprintf(
          stderr ,
          "nonet error : failed to 'ip link set dev lo up': %d\n" ,
          es
        );
        
        return FAIL_LO_UP_ERROR ;
      }
      
      argv ++ ;
    }
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
