#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <asm/delay.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "ku_ipc.h"


#define DEV_NAME "ku_ipc_dev"

MODULE_LICENSE("GPL");
struct msgs *user_buf;
int wait_param[10];
spinlock_t my_lock;
struct msgs{
  int msgqid;
  void* msgp;
  int msgsz;
  long msgtype;
  int msgflg;
  int return_val;
};


wait_queue_head_t my_wq[10];
wait_queue_head_t my_wq_rcv[10];
struct msg_buf{
  long type;
  char str[128];
};

struct msg_list{
 struct list_head list;
int ref;
int msg_vol;
int msg_count;
 struct msg_buf msg;
};

spinlock_t my_lock;

struct msg_list msg_lists[10];

void delay(int sec)
{
int i,j;
for(j=0;j<sec;j++)
{
    for(i=0;i<1000;i++)
    {
        udelay(1000);
    }
}

}

int ku_msgget(int key,int msgflg)
{
//message create
//printk("msgflg %d ku_ipc %d ref",msgflg,KU_IPC_CREAT,msg_lists[key].ref);

if((msgflg&KU_IPC_CREAT)&&(msg_lists[key].ref==0))
{

msg_lists[key].ref++;
return key;
}
if(msgflg&KU_IPC_EXCL)
{
    if(msg_lists[key].ref==0)
    {
    msg_lists[key].ref++;
    return key;
    }
}

return -1;

}

int ku_msgclose(int msqid)
{
if(msg_lists[msqid].ref==0) return-1;
else{
 msg_lists[msqid].ref--;
 return 0;
}

}
int ku_msgsnd(int msqid,void *msgp,int msgsz,int msgflg)
{
//send  user->kernel
//write cmd
struct msg_list *tmp=0;
struct list_head *pos=0;
printk("ku_msgsend start\n");

printk("msg_lists[%d] : count %d\n",msqid,msg_lists[msqid].msg_count);
printk("ere %d\n",msg_lists[msqid].msg_vol);
if((msg_lists[msqid].msg_count>=KUIPC_MAXMSG)||(msg_lists[msqid].msg_vol>=KUIPC_MAXVOL))
{
   
    if((msgflg&KU_IPC_NOWAIT)!=0)  //-> return
    {
    return -1;
    }
    while((msg_lists[msqid].msg_vol+msgsz>=KUIPC_MAXVOL)||(msg_lists[msqid].msg_count>=KUIPC_MAXMSG))
    {
    printk("wait\n");
    wait_event_interruptible(my_wq[msqid],((msg_lists[msqid].msg_vol+msgsz)<KUIPC_MAXVOL)&&(msg_lists[msqid].msg_count<KUIPC_MAXMSG));
    
    spin_lock(&my_lock);
    if(msg_lists[msqid].msg_count<KUIPC_MAXMSG)
    {
    msg_lists[msqid].msg_count++;
    msg_lists[msqid].msg_vol+=msgsz;
    tmp=(struct msg_list*)kmalloc(sizeof(struct msg_list),GFP_KERNEL);
    copy_from_user(&tmp->msg,(struct msg_buf*)msgp,sizeof(struct msg_buf));
    tmp->msg.type=1;
    printk("%s\n",tmp->msg.str);
    list_add_tail(&tmp->list,&msg_lists[msqid].list);
    wake_up_interruptible(&my_wq_rcv[msqid]);
    spin_unlock(&my_lock);
    return 0;   
    }
    spin_unlock(&my_lock);
    
    }
}
//msg_count<max
spin_lock(&my_lock);
msg_lists[msqid].msg_count++;
msg_lists[msqid].msg_vol+=msgsz;
tmp=(struct msg_list*)kmalloc(sizeof(struct msg_list),GFP_KERNEL);
copy_from_user(&tmp->msg,(struct msg_buf*)msgp,sizeof(struct msg_buf));
tmp->msg.type=1;
printk("%s why\n",tmp->msg.str);
list_add_tail(&tmp->list,&msg_lists[msqid].list);    
spin_unlock(&my_lock);
wake_up_interruptible(&my_wq_rcv[msqid]);
return 0;


}


int ku_msgrcv(int msqid,void *msgp,int msgsz,long msgtyp,int msgflg)
{
    int msg_size;
struct list_head *pos=0;
struct list_head *q=0;
struct msg_list *tmp=0;
unsigned int sum=0;
//one message send!!

if(list_empty(&msg_lists[msqid].list))
{
     
    if((msgflg&KU_IPC_NOWAIT)==0)
    {
 
    wait_event_interruptible_exclusive(my_wq_rcv[msqid],msg_lists[msqid].msg_count>0);
    }
    else
    {
   //Ku_ipc_nowait !=0;
    return -1;
    }
}
printk("start rcv\n");
list_for_each_safe(pos,q,&msg_lists[msqid].list)
    {
    tmp=list_entry(pos,struct msg_list,list);
    if(tmp->msg.type==msgtyp)
    {
    if(msgsz<strlen(tmp->msg.str))
    {
    if((msgflg&KU_IPC_NOERROR)==0) return -1;

       tmp->msg.str[msgsz]='\0';
    }
   if((msgflg&KU_IPC_NOERROR)!=0||(msgflg&KU_IPC_NOWAIT)!=0)
    {
    
    spin_lock(&my_lock);
    msg_lists[msqid].msg_count--;
    msg_lists[msqid].msg_vol=msg_lists[msqid].msg_vol-msgsz;
    copy_to_user((struct msg_buf*)msgp,&(tmp->msg),sizeof(struct msg_buf));
     printk("message : %s\n",tmp->msg.str);
     msg_size=strlen(tmp->msg.str);
     printk("mesg size: %d\n",strlen(tmp->msg.str));
     wake_up_interruptible(&my_wq[msqid]);

     list_del(pos);
 
    kfree(tmp);
    spin_unlock(&my_lock);
    return strlen(tmp->msg.str);
    
    }
    else{

 return -1;
    }

    }
    
    }

// have not equal type
return -1;
}






static int ku_ipc_open(struct inode*inode,struct file *file)
{
 printk(" open\n");
 return 0;

}

static int ku_ipc_release(struct inode*inode,struct file *file){
printk("ku ipc: release \n");
return 0;

}

static long ku_ipc_ioctl(struct file *file,unsigned int cmd,unsigned long arg)
{


int ret;
user_buf=(struct msgs*)arg;

switch(cmd){
        case KU_MSGSND:
                ret=ku_msgsnd(user_buf->msgqid,user_buf->msgp,user_buf->msgsz,user_buf->msgflg);
                copy_to_user(&user_buf->return_val,&ret,sizeof(int));
                break;
        case KU_MSGRCV:
                ret=ku_msgrcv(user_buf->msgqid,user_buf->msgp,user_buf->msgsz,user_buf->msgtype,user_buf->msgflg);
                copy_to_user(&user_buf->return_val,&ret,sizeof(int));
                break;
        case KU_MSGGET:
                ret=ku_msgget(user_buf->msgqid,user_buf->msgflg);
                copy_to_user(&user_buf->return_val,&ret,sizeof(int));
                break;
        case KU_MSGCLOSE:
                ret=ku_msgclose(user_buf->msgqid);
                copy_to_user(&user_buf->return_val,&ret,sizeof(int));
                break;

        
        default:
                return -1;

}
return 0;

}

static dev_t dev_num;
static struct cdev *cd_cdev;

 
struct file_operations ku_ipc_fops={
    .open=ku_ipc_open,
    .release=ku_ipc_release,
    .unlocked_ioctl=ku_ipc_ioctl,
};





static int __init hello_init(void)
{
printk("init!!!GHKRDLS\n");
struct msg_st *tmp=0;
struct list_head *pos=0;
unsigned int i;
alloc_chrdev_region(&dev_num,0,1,DEV_NAME);
cd_cdev=cdev_alloc();
cdev_init(cd_cdev,&ku_ipc_fops);
cdev_add(cd_cdev,dev_num,1);
spin_lock_init(&my_lock);
//reset
for(i=0;i<10;i++)
{
INIT_LIST_HEAD(&msg_lists[i].list);
msg_lists[i].ref=0;
msg_lists[i].msg_vol=0;
msg_lists[i].msg_count=0;
init_waitqueue_head(&my_wq[i]);

init_waitqueue_head(&my_wq_rcv[i]);

}
int ret;


return 0;


}
static void __exit hello_exit(void)
{
    printk(" : exit goodbyer\n");
    cdev_del(cd_cdev);
    unregister_chrdev_region(dev_num,1);
    
 struct msg_list *tmp=0;
struct list_head *pos=0;
struct list_head *q=0;
unsigned int z;
for(z=0;z<10;z++)
{
list_for_each_safe(pos,q,&msg_lists[z].list)
{
tmp=list_entry(pos,struct msg_list,list);
list_del(pos);
kfree(tmp);

}
}    
}


module_init(hello_init);
module_exit(hello_exit);


