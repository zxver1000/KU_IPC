#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "ku_ipc.h"
#include <string.h>

struct msgs{
  int msgqid;
  void* msgp;
  int msgsz;
  long msgtype;
  int msgflg;
  int return_val;
};
struct msg_buf{
  long type;
  char str[128];
};

int ku_msgget(int key,int msgflg)
{
 int fd;
fd=open("/dev/ku_ipc_dev",O_RDWR);
 struct msgs msg;
 msg.msgqid=key;
 msg.msgflg=msgflg;


ioctl(fd,KU_MSGGET,&msg);

if(msg.return_val==-1) 
{
  printf("fail\n");
return -1;
}

printf("success key\n");
  return key;
}

int ku_msgclose(int msqid)
{
  int fd;
fd=open("/dev/ku_ipc_dev",O_RDWR);
 struct msgs msg;
 msg.msgqid=msqid;
ioctl(fd,KU_MSGCLOSE,&msg);
if(msg.return_val==-1) return -1;
//success
return 0;

}



int ku_msgrcv(int msqid,void *msgp,int msgsz,long msgtyp,int msgflg)
{
  int fd;
  fd=open("/dev/ku_ipc_dev",O_RDWR);
 int val;
 struct msgs rcv_msg;
 rcv_msg.msgqid=msqid;
 rcv_msg.msgp=msgp;
 rcv_msg.msgsz=msgsz;
 rcv_msg.msgtype=msgtyp;
 rcv_msg.msgflg=msgflg;


ioctl(fd,KU_MSGRCV,&rcv_msg);


val=rcv_msg.return_val;
if(val==-1) return -1;

return val;
}

int ku_msgsend(int msqid,void *msgp,int msgsz,int msgflg)
{
int fd;
int val;
fd=open("/dev/ku_ipc_dev",O_RDWR);

 struct msgs snd_msg;
 snd_msg.msgqid=msqid;
 snd_msg.msgp=msgp;
 snd_msg.msgsz=msgsz;
 snd_msg.msgflg=msgflg;

ioctl(fd,KU_MSGSND,&snd_msg);
if(snd_msg.return_val==-1)
{
  printf("fail snd\n");
  return -1;
}

printf("success snd\n");
return 0;
}


int main(void){

int fd,ret,cmd;

char zd[128];

fd=open("/dev/ku_ipc_dev",O_RDWR);
struct msg_buf alpa;
struct msgs msg={
   0,&alpa,0,1,0,0
};

int n;
printf("choose msqid\n");
scanf("%d",&n);
msg.msgqid=n;
int num;
printf("choose flag 1.KU_IPC_CREAT 2.KU_IPC_EXCL :\n");
scanf("%d",&num);
int flags;
if(num==1) flags=KU_IPC_CREAT; 
else if(num==2) flags=KU_IPC_EXCL;
else return-1;
msg.msgflg=flags;
int get;
struct msg_buf snds;
struct msg_buf rcvs;
int val;
get=ku_msgget(msg.msgqid,msg.msgflg);
while(get==-1)
{
  printf("it is key not available now choose different key\n");
  scanf("%d",&msg.msgqid);
get=ku_msgget(msg.msgqid,msg.msgflg);
}
ku_msgclose(msg.msgqid);
char snd_msg_buf[128];


  printf("choose 1.msgsnd,2.msgrecive else-> exit\n");
  scanf("%d",&num);
if(num==1)
{

 printf("choose flag 1.NOWAIT 2.0 :\n");
 scanf("%d",&num);
 if(num==1) msg.msgflg=KU_IPC_NOWAIT;
 else msg.msgflg=0;
 printf("send message\n");
 scanf("%s",snds.str);
  
 val=ku_msgsend(msg.msgqid,&snds,128,msg.msgflg);
 if(val==-1)
 {
   printf("send fail\n");
 }
 else{
    printf("snd  message : %s\n",snds.str);
 }
 snds.str[0]='\0';
}
else if(num==2)
{
 
 printf("choose flag 1.NOWAIT 2.NOERROR:\n");
 scanf("%d",&num);
if(num==1) msg.msgflg=KU_IPC_NOWAIT;
else msg.msgflg=KU_IPC_NOERROR;


printf("choose receive_message length\n");
scanf("%d",&num);

val=ku_msgrcv(msg.msgqid,&rcvs,num,msg.msgtype,msg.msgflg);
printf("msg length : %d \n",val);
if(val!=-1)
{
  printf("rcv_msg is %s \n",rcvs.str);
}
else printf("fail -1\n");

rcvs.str[0]='\0';
}
else{
  //break;
}

//ku_msgclose(msg.msgqid);
close(fd);

return 1;
}