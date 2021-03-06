#include <linux/types.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <linux/path.h>
#include <linux/list.h>
struct inf_dev {
  struct cdev cdev;
};
void print_process_tree(struct task_struct *current_task, int num);
void print_thread_group(void);
void print_mem_stat(pid_t pid);
void print_task_fs(void);
static int inf_init_module(void);
static void inf_cleanup_module(void);
static int inf_open(struct inode *inode, struct file *filp);
static int inf_release(struct inode *inode, struct file *filp);
static ssize_t inf_read(struct file *filp, char *buf, size_t count,
    loff_t *f_pos);
static ssize_t inf_write(struct file *filp, const char *buf, size_t count,
    loff_t *f_pos);
static loff_t inf_llseek(struct file *filp, loff_t off, int whence);
static int inf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
/*operation function*/
static struct file_operations inf_fops = { 
	.owner = THIS_MODULE, 
	.llseek =    inf_llseek, 
	.read = inf_read, 
	.write = inf_write, 
	.unlocked_ioctl =inf_ioctl, 
    	.open = inf_open, 
    	.release = inf_release, 
    };
struct inf_dev *inf_devices;
struct task_struct *first_task = NULL;
static unsigned char inf_inc = 0;
#define maxsize 409600
u8 infBuffer[maxsize];
int str_len = 0;
char *p = infBuffer;
pid_t given_pid = 0;
#define INF_MAJOR 250
#define INF_MINOR 0
#define COMMAND1 0
#define COMMAND2 1
#define COMMAND3 8
#define COMMAND4 16
void print_process_tree(struct task_struct *current_task, int num) {
  int i = 0;
  int j;
  int k;
  struct task_struct *task;
  struct list_head *list;
  for (k = 0; k < maxsize; k++) {
    infBuffer[k] = '\0';
  }
  p = infBuffer;
  list_for_each(list, &current_task->children)
  {
    task = list_entry(list,struct task_struct,sibling);
    //task现在指向当前的某个子进程
    for (j = 0; j <= num; j++) {
      sprintf(p, "--");
      p = p + 2;
      str_len = str_len + 2;
    }
    printk("%16s[%4d]\n", task->comm, task->pid);
    sprintf(p, "%16s[%4d]\n", task->comm, task->pid);
    i = strlen(p);
    str_len = str_len + i;
    p = p + i;
    num++;
    print_process_tree(task, num);
    num--;
  }
}
void print_thread_group(void) {
  int i = 0;
  struct task_struct *task_process;
  struct task_struct *task_thread;
  struct list_head *list;
  int j;
  for (j = 0; j < maxsize; j++) {
    infBuffer[j] = '\0';
  }
  p = infBuffer;
  for_each_process(task_process)
  {
    printk("%16s[%4d]\n", task_process->comm, task_process->pid);
    sprintf(p, "%16s[%4d]\n", task_process->comm, task_process->pid);
    i = strlen(p);
    str_len = str_len + i;
    p = p + i;
    list_for_each(list, &task_process->thread_group)
    {
      task_thread=list_entry(list,struct task_struct,thread_group);
      printk("%16s[%4d]\n", task_thread->comm, task_process->pid);
      sprintf(p, "------%16s[%4d]\n", task_thread->comm,
          task_process->pid);
      i = strlen(p);
      str_len = str_len + i;
      p = p + i;
    }
  }
}
void print_task_fs(void) {
  struct dentry *d;
  struct dentry *r;
  struct dentry *t;
  struct inode *in;
  //struct dentry *subdirs;
  //struct list_head *list;
  //struct list_head *temp;
  int i = 0;
  struct task_struct *task_process;
  int j;

  for (j = 0; j < maxsize; j++) {
    infBuffer[j] = '\0';
  }
  p = infBuffer;
  for_each_process(task_process)
  {

    if (task_process->fs == NULL) {
      sprintf(p, "The process %d does not have related information!\n",
          task_process->pid);
      i = strlen(p);
      str_len = str_len + i;
      p = p + i;
    } else {
      d = task_process->fs->pwd.dentry;
      r = task_process->fs->root.dentry;
      sprintf(p, "--------------------------------------------\n");
      i = strlen(p);
      str_len = str_len + i;
      p = p + i;

      do {
        t = d;
        in = d->d_inode;
        sprintf(p, "pwd--pwd:%10s \t\n", d->d_name.name);

        i = strlen(p);
        str_len = str_len + i;
        p = p + i;
        /*
         list=&(d->d_subdirs);
         temp=list;
         list=list->next;
         
         while(list!=&(d->d_subdirs)&&list!=NULL)
         {    
         subdirs=list_entry(list,struct dentry,d_u.d_child);
         sprintf(p,"child--child:%10s \t\n",subdirs->d_name.name);
         i=strlen(p);
         str_len=str_len+i;
         p=p+i;
         list=list->next;
         }
         */
        d = d->d_parent;
        /*
         sprintf(p,"parent--parent:%10s \n",d->d_name.name);
         i=strlen(p);
         str_len=str_len+i;
         p=p+i;
         */
      } while (d != t);

      sprintf(p, "root:%s \t", r->d_name.name);
      i = strlen(p);
      str_len = str_len + i;
      p = p + i;
      sprintf(p, "pid:%d \t pwd:%s \n", task_process->pid,
          task_process->fs->pwd.dentry->d_name.name);
      i = strlen(p);
      str_len = str_len + i;
      p = p + i;
    }
  }
}
void print_mem_stat(pid_t pid) {
  int i = 0;
  struct vm_area_struct *vm_area_p = NULL;
  struct task_struct *task_process;
  for_each_process(task_process)
  {
    if (task_process->pid == pid) {
      int j;
      for (j = 0; j < maxsize; j++) {
        infBuffer[j] = '\0';
      }
      p = infBuffer;
      if (task_process->mm == NULL) {
        sprintf(p, "The process dose not have related information!\n");
        i = strlen(p);
        str_len = str_len + i;
        p = p + i;
      } else {
        sprintf(p,
            "code section address:\n\t start: 0x%lx \t end: 0x%lx \t size: 0x%lx\n",
            task_process->mm->start_code,
            task_process->mm->end_code,
            (task_process->mm->end_code
                - task_process->mm->start_code));
        i = strlen(p);
        str_len = str_len + i;
        p = p + i;
        sprintf(p,
            "data section address:\n\t start: 0x%lx \t end: 0x%lx \t size: 0x%lx\n",
            task_process->mm->start_data,
            task_process->mm->end_data,
            (task_process->mm->end_data
                - task_process->mm->start_data));
        i = strlen(p);
        str_len = str_len + i;
        p = p + i;
        sprintf(p,
            "bss section address:\n\t start: 0x%lx \t end: 0x%lx \t size: 0x%lx\n",
            task_process->mm->end_data, task_process->mm->start_brk,
            (task_process->mm->start_brk
                - task_process->mm->end_data));
        i = strlen(p);
        str_len = str_len + i;
        p = p + i;
        sprintf(p,
            "brk section address:\n\t start: 0x%lx \t end: 0x%lx \t size: 0x%lx\n",
            task_process->mm->start_brk, task_process->mm->brk,
            (task_process->mm->brk - task_process->mm->start_brk));
        i = strlen(p);
        str_len = str_len + i;
        p = p + i;
        sprintf(p,
            "remainder stack section address:\n\t start: 0x%lx \t end: 0x%lx \t size: 0x%lx\n",
            task_process->mm->brk, task_process->mm->start_brk,
            (task_process->mm->start_brk - task_process->mm->brk));
        i = strlen(p);
        str_len = str_len + i;
        p = p + i;
        sprintf(p,
            "arg section address:\n\t start: 0x%lx \t end: 0x%lx \t size: 0x%lx\n",
            task_process->mm->arg_start, task_process->mm->arg_end,
            (task_process->mm->arg_end - task_process->mm->arg_start));
        i = strlen(p);
        str_len = str_len + i;
        p = p + i;
        sprintf(p,
            "env section address:\n\t start: 0x%lx \t end: 0x%lx \t size: 0x%lx\n",
            task_process->mm->env_start, task_process->mm->env_end,
            (task_process->mm->env_end - task_process->mm->env_start));
        i = strlen(p);
        str_len = str_len + i;
        p = p + i;
        sprintf(p,
            "vm_area------------------------------------------------------------vm_area\n");
        i = strlen(p);
        str_len = str_len + i;
        p = p + i;
        for (vm_area_p = task_process->mm->mmap; vm_area_p != NULL;
            vm_area_p = vm_area_p->vm_next) {
          sprintf(p, "\n\t start: 0x%lx \t end: 0x%lx \t\n",
              vm_area_p->vm_start, vm_area_p->vm_end);
          i = strlen(p);
          str_len = str_len + i;
          p = p + i;
        }
      }
      break;
    }
  }
}
static int inf_init_module(void) {
  int result;
  dev_t dev = 0;
  dev = MKDEV(INF_MAJOR, INF_MINOR);
  result = register_chrdev(0, "inf", &inf_fops);
  if (result < 0) {
    printk(KERN_WARNING "inf: can't get major %d\n", INF_MAJOR);
    return result;
  }
  //alloct inf_dev
  inf_devices = kmalloc(sizeof(struct inf_dev), GFP_KERNEL);
  if (!inf_devices) {
    result = -ENOMEM;
    goto fail;
  }
  memset(inf_devices, 0, sizeof(struct inf_dev));
  //init a char driver
  cdev_init(&inf_devices->cdev, &inf_fops);
  inf_devices->cdev.owner = THIS_MODULE;
  inf_devices->cdev.ops = &inf_fops;
  //add char driver to kernel
  result = cdev_add(&inf_devices->cdev, dev, 1);
  if (result) {
    printk(KERN_NOTICE "Error %d addint inf\n", result);
    goto fail;
  }
  return 0;
//handle fail
  fail: inf_cleanup_module();
  return result;
}

static void inf_cleanup_module(void) {
  dev_t devno = MKDEV(INF_MAJOR, INF_MINOR);
  //delete driver
  if (inf_devices) {
    cdev_del(&inf_devices->cdev);
    kfree(inf_devices);
  }
  unregister_chrdev_region(devno, 1);
}
module_init( inf_init_module);
module_exit( inf_cleanup_module);
//open function
static int inf_open(struct inode *inode, struct file *filp) {
  struct inf_dev *dev;
  if (inf_inc > 0)
    return -ERESTARTSYS;
  inf_inc++;
  dev = container_of(inode->i_cdev, struct inf_dev, cdev);
  filp->private_data = dev;
  return 0;
}
//release function
static int inf_release(struct inode *inode, struct file *filp) {
  inf_inc--;
  return 0;
}
//read function
static ssize_t inf_read(struct file *filp, char *buf, size_t count,
    loff_t *f_pos) {
  int result;
  loff_t pos = *f_pos;
  if (pos >= maxsize) {
    result = 0;
    goto out;
  }
  if (count > 40960 - pos) {
    count = 40960 - pos;
  }
  pos += count;
  //write data to user space
  if (copy_to_user(buf, infBuffer + *f_pos, count)) {
    count = -EFAULT;
    goto out;
  }
  *f_pos = pos;
  out: return count;
}
//write function
static ssize_t inf_write(struct file *filp, const char *buf, size_t count,
    loff_t *f_pos) {
  ssize_t retval = -ENOMEM;
  loff_t pos = *f_pos;
  if (pos > maxsize) {
    goto out;
  }
  if (count > maxsize - pos) {
    count = maxsize - pos;
  }
  pos += count;
  if (copy_from_user(infBuffer + *f_pos, buf, count)) {
    retval = -EFAULT;
    goto out;
  }
  *f_pos = pos;
  return count;
  out: return retval;
}
//llseek function
static loff_t inf_llseek(struct file *filp, loff_t off, int whence) {
  loff_t pos;
  pos = filp->f_pos;
  switch (whence) {
  case 0:
    pos = off;
    break;
  case 1:
    pos += off;
    break;
  case 2:
  default:
    return -EINVAL;
  }
  if (pos > maxsize || pos < 0) {
    return -EINVAL;
  }
  return filp->f_pos = pos;
}
//ioctl function
static int inf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  printk("Command:%d\n", cmd);
  if (cmd == COMMAND1) {
    int i = 0;
    printk("ioctl command1 successfully\n");
    first_task = &init_task;
    sprintf(p, "%16s[%4d]\n", first_task->comm, first_task->pid);
    i = strlen(p);
    str_len = str_len + i;
    p = p + i;
    print_process_tree(first_task, 0);
    return 0;
  }
  if (cmd == COMMAND2) {
    printk("ioctl command2 successfully\n");
    print_thread_group();
    return 0;
  }
  if (cmd == COMMAND3) {
    printk("ioctl command3 successfully\n");
    given_pid = arg;
    print_mem_stat(given_pid);
    return 0;
  }
  if (cmd == COMMAND4) {
    printk("ioctl command4 successfully\n");
    print_task_fs();
    return 0;
  }
  printk("ioctl error\n");
  return -EFAULT;
}
