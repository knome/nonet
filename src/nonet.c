
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
    fprintf( stderr, "nonet error : could not unshare netns : %d : %s\n", errno, strerror( errno ) );
    return FAIL_INVOCATION_ERROR ;
  }
  
  if( setegid( getgid() ) == -1 ){
    fprintf( stderr, "failed to setegid to getgid, %s", strerror( errno ) );
    return FAIL_INVOCATION_ERROR ;
  }
  
  if( seteuid( getuid() ) == -1 ){
    fprintf( stderr, "failed to seteuid to getuid, %s", strerror( errno ) );
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
