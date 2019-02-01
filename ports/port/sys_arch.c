#include "lwip/debug.h"

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>


#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip_ctxt.h"
#include "os_ctxt.h"


simulation_context sim_ctxt;
// Global start time for all system nodes, used in sys_now to get the absolute execution time stamp
static sc_dt::uint64 starttime;

#if !NO_SYS
static struct sys_thread *threads = NULL;
static sc_core::sc_mutex threads_mutex;

struct sys_mbox_msg {
  struct sys_mbox_msg *next;
  void *msg;
};

#define SYS_MBOX_SIZE 128
struct sys_mbox {
  int id;
  int first, last;
  void *msgs[SYS_MBOX_SIZE];
  struct sys_sem *not_empty;
  struct sys_sem *not_full;
  struct sys_sem *mutex;
  int wait_send;
  void * ctxt;

};

#define GLOBAL_SEMS 200
struct sys_sem {
  bool free;
  int id;
  unsigned int c;
  sc_core::sc_event cond; // systemc channel
  sc_core::sc_mutex mutex; // systemc mutex
  void * ctxt;
  int blocking_task_id;
  int blocked_task_id;
} sems[GLOBAL_SEMS];

int global=0;
int sems_init=0;//Can not dynamically allocate sems

struct sys_thread {
  struct sys_thread *next;
  sc_core::sc_process_handle sc_thread;
};

#if SYS_LIGHTWEIGHT_PROT
static sc_core::sc_mutex lwprot_mutex;
static sc_core::sc_process_handle lwprot_thread;
static sc_core::sc_process_handle SC_DEAD_PROCESS = sc_core::sc_process_handle();
static int lwprot_count = 0;
#endif /* SYS_LIGHTWEIGHT_PROT */

static struct sys_sem *sys_sem_new_internal(u8_t count);
static void sys_sem_free_internal(struct sys_sem *sem);


/*Function wrapper for OS task model*/
void wrapper(os_model_context* os_ctxt, void *ctxt, thread_fn function, void *arg, int taskID){
	sim_ctxt.register_task(os_ctxt, ctxt, taskID, sc_core::sc_get_current_process_handle());
	os_ctxt->os_port->taskActivate(taskID);
	function(arg);//All lwip thread args are converted to context object pointers.
 	os_ctxt->os_port->taskTerminate(taskID);
}

typedef void (*OS_wrapper_fn)(os_model_context* os_ctxt, void *ctxt, thread_fn function, void* arg, int taskID);


sys_thread_t
sys_thread_new(const char *name, thread_fn function, void *arg, int affinity, int core)
{

  struct sys_thread *thread_p;
  //for(thread_p=threads; thread_p!=NULL; thread_p=thread_p->next)
  //	{
  //	 std::cout<<thread_p->sc_thread.name()<<std::endl;
  //	 std::cout<<thread_p->sc_thread.valid()<<std::endl;
  //	 std::cout<<thread_p->sc_thread.dynamic()<<std::endl;
  //}
  int code;
  void* ctxt;
  ctxt = sim_ctxt.get_app_ctxt(sc_core::sc_get_current_process_handle())->get_context("lwIP");//HCSim
  sc_core::sc_process_handle th_handle;
  struct sys_thread *thread = NULL;
  LWIP_UNUSED_ARG(name);
  LWIP_UNUSED_ARG(affinity);

  int child_id; 
  const sc_dt::uint64 exe_cost = 8006000ull;
  const sc_dt::uint64 period = 49000000ull;
  const unsigned int priority = 49;
  const int init_core = core;


  os_model_context* os_ctxt = sim_ctxt.get_os_ctxt(sc_core::sc_get_current_process_handle());
  child_id = os_ctxt->os_port->taskCreate(
				sc_core::sc_gen_unique_name("child_task"), 
				HCSim::OS_RT_APERIODIC, priority, period, exe_cost, 
				HCSim::DEFAULT_TS, HCSim::ALL_CORES, init_core);

  os_ctxt->os_port->dynamicStart(init_core);
  OS_wrapper_fn OS_fn = NULL;
  OS_fn = &wrapper;
  th_handle = sc_core::sc_spawn(     
                                 sc_bind(  
                                         OS_fn,
                                         os_ctxt, ctxt, function, arg, child_id 
                                         )         
                                ); 

  thread = new sys_thread;
  if (thread != NULL) {
    //((lwip_context*)ctxt)->threads_mutex.lock();
    threads_mutex.lock();
    thread->next = ((lwip_context*)ctxt)->threads;
    thread->sc_thread = th_handle;
    threads = thread;
    threads_mutex.unlock();
   // ((lwip_context*)ctxt)->threads_mutex.unlock();
  }
  //printf("/***************New thread name***************/: %s\n", name);
  //std::cout << "Thread ID in OS model is: " << child_id << std::endl; 
  //std::cout << name << std::endl;
  //std::cout << "On core: "<< core << std::endl;
  //printf("/***************New thread name***************/: %s\n", name);
  return thread;
}

/*-----------------------------------------------------------------------------------*/
err_t
sys_mbox_new(struct sys_mbox **mb, int size)
{
  int taskID = sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle());
  //( (lwip_context*)(ctxt) ) -> getOSModelTaskID( sc_core::sc_get_current_process_handle());

  //std::cout << " ************ "<<taskID<<" ************Creating mbox task ID is: ... ... .." << taskID <<std::endl;
  //Debug mark: OS Model Integration
  
  struct sys_mbox *mbox;
  LWIP_UNUSED_ARG(size);

  //mbox = (sys_mbox *)new(sizeof( sys_mbox));
  mbox = new sys_mbox;

  if (mbox == NULL) {
    return ERR_MEM;
  }
  mbox->id = taskID;
  //mbox->ctxt = ctxt;
  mbox->first = mbox->last = 0;
  mbox->not_empty = sys_sem_new_internal(0);
  mbox->not_full = sys_sem_new_internal(0);
  mbox->mutex = sys_sem_new_internal(1);
  mbox->wait_send = 0;
  SYS_STATS_INC_USED(mbox);
  *mb = mbox;

  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
void
sys_mbox_free(struct sys_mbox **mb)
{
  if ((mb != NULL) && (*mb != SYS_MBOX_NULL)) {
    struct sys_mbox *mbox = *mb;
    SYS_STATS_DEC(mbox.used);


    sys_sem_free_internal(mbox->not_empty);
    sys_sem_free_internal(mbox->not_full);
    sys_sem_free_internal(mbox->mutex);
    mbox->not_empty = mbox->not_full = mbox->mutex = NULL;
    /*  LWIP_DEBUGF("sys_mbox_free: mbox 0x%lx\n", mbox); */
    delete mbox;
  }
}
/*-----------------------------------------------------------------------------------*/
err_t
sys_mbox_trypost(struct sys_mbox **mb, void *msg)
{

  u8_t first;
  struct sys_mbox *mbox;
  LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));

  mbox = *mb;

  int taskID = sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle());
  //( (lwip_context*)(mbox->ctxt) ) -> getOSModelTaskID( sc_core::sc_get_current_process_handle());
  //if(mbox->id == 111)std::cout << " ************ "<<taskID<<"w************try writing mbox ID is: ... ... .." << mbox->id <<std::endl;


  LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_trypost: mbox %p msg %p\n",
                          (void *)mbox, (void *)msg));

  if ((mbox->last + 1) >= (mbox->first + SYS_MBOX_SIZE)) {
    //sys_sem_signal(&mbox->mutex);
    printf("Returning with memory error!\n");
    return ERR_MEM;
  }

  mbox->msgs[mbox->last % SYS_MBOX_SIZE] = msg;

  if (mbox->last == mbox->first) {
    first = 1;
  } else {
    first = 0;
  }

  mbox->last++;

  if (first) {
    //if(( (lwip_context*)((mbox->not_empty) -> ctxt) )-> NodeID == 1) printf("sys_sem_signal(&mbox->not_empty);%d sem %d mbox %p msg %p\n", ( (lwip_context*)((mbox->not_empty) -> ctxt) )-> NodeID ,(mbox->not_empty)->id,(void *)mbox, (void *)msg);
    sys_sem_signal(&mbox->not_empty);

  }

 // sys_sem_signal(&mbox->mutex);

  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
void
sys_mbox_post(struct sys_mbox **mb, void *msg)
{
  u8_t first;
  struct sys_mbox *mbox;
  LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
  mbox = *mb;
  int taskID = sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle());
  LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post: mbox %p msg %p\n", (void *)mbox, (void *)msg));
  while ((mbox->last + 1) >= (mbox->first + SYS_MBOX_SIZE)) {
    mbox->wait_send++;
    sys_arch_sem_wait(&mbox->not_full, 0);
    mbox->wait_send--;
  }
  mbox->msgs[mbox->last % SYS_MBOX_SIZE] = msg;
  if (mbox->last == mbox->first) {
    first = 1;
  } else {
    first = 0;
  }
  mbox->last++;
  if (first) {
    sys_sem_signal(&mbox->not_empty);
  }
}
/*-----------------------------------------------------------------------------------*/
u32_t
sys_arch_mbox_tryfetch(struct sys_mbox **mb, void **msg)
{
  struct sys_mbox *mbox;
  LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
  mbox = *mb;
  int taskID = sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle());
  if (mbox->first == mbox->last) {
    return SYS_MBOX_EMPTY;
  }
  if (msg != NULL) {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_tryfetch: mbox %p msg %p\n", (void *)mbox, *msg));
    *msg = mbox->msgs[mbox->first % SYS_MBOX_SIZE];
  }
  else{
    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_tryfetch: mbox %p, null msg\n", (void *)mbox));
  }

  mbox->first++;
  if (mbox->wait_send) {
    sys_sem_signal(&mbox->not_full);
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
u32_t
sys_arch_mbox_fetch(struct sys_mbox **mb, void **msg, u32_t timeout)
{

  u32_t time_needed = 0;
  struct sys_mbox *mbox;
  LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
  mbox = *mb;
  int taskID = sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle());
  while (mbox->first == mbox->last) {
    if (timeout != 0) {
      time_needed = sys_arch_sem_wait(&mbox->not_empty, timeout);
      if (time_needed == SYS_ARCH_TIMEOUT) {
        return SYS_ARCH_TIMEOUT;
      }
    } else {
      sys_arch_sem_wait(&mbox->not_empty, 0);
    }
  }

  if (msg != NULL) {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_fetch: mbox %p msg %p\n", (void *)mbox, *msg));
    *msg = mbox->msgs[mbox->first % SYS_MBOX_SIZE];
  }else{
    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_fetch: mbox %p, null msg\n", (void *)mbox));
  }

  mbox->first++;

  if (mbox->wait_send) {
    sys_sem_signal(&mbox->not_full);
  }

  //sys_sem_signal(&mbox->mutex);
  //if(( (lwip_context*)((mbox->not_empty) -> ctxt) )-> NodeID == 1) printf("return .. .... ..... sys_arch_mbox_fetch\n");
  return time_needed;
}

static struct sys_sem *
sys_sem_new_internal(u8_t count)
{ 
  int i = 0;
  if(sems_init == 0){
	sems_init = 1;
	for(i = 0; i<GLOBAL_SEMS; i++){
		sems[i].free = 1;
	}
  }


 //
  for(i = 0; i<GLOBAL_SEMS; i++){
	if(sems[i].free == 1){
		break;
	}
  }

  global=i;

  struct sys_sem *sem;
  sem = sems + global;
  sem->id = global;
  sem->blocking_task_id = -1;
  sem->blocked_task_id = -1; 
  sem->free=0;
  if (GLOBAL_SEMS==global){printf("global  is %d \n", global);}
  if (sem != NULL) {
    sem->c = count;
  }
  return sem;
}

err_t sys_sem_new(struct sys_sem **sem, u8_t count)
{ 
  SYS_STATS_INC_USED(sem);
  *sem = sys_sem_new_internal(count);
  if (*sem == NULL) {
    return ERR_MEM;
  }
  return ERR_OK;
}

u32_t sys_arch_sem_wait(struct sys_sem **s, u32_t timeout)
{

  sc_dt::uint64 start_time;
  u32_t time_needed = 0;
  struct sys_sem *sem;
  LWIP_ASSERT("invalid sem", (s != NULL) && (*s != NULL)); 
  sem = *s;
  int taskID = sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle());

  sem->blocked_task_id = taskID;
  while (sem->c <= 0) {
        { ( sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() ) ) -> os_port -> preWait(taskID, sem->blocking_task_id);}	
        if(sem->c > 0){
                (  sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() ) ) ->os_port->postWait(taskID);
                break;
        }

        if (timeout > 0) {
           start_time = sc_core::sc_time_stamp().value();
           sc_core::wait(timeout, sc_core::SC_MS, sem->cond);
           //sc_core::wait(sem->cond);
           start_time = (sc_core::sc_time_stamp().value() - start_time);
	   time_needed = (u32_t)(start_time/1000000000);
	   if((time_needed == timeout) && (sem->c <= 0)){
        	(  sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() ) ) ->os_port->postWait(taskID);
	   	return SYS_ARCH_TIMEOUT;
           }
	}else{
	   //printf("sc_core::wait(sem->cond) ... ...%d \n\n", ((lwip_context*)(sem->ctxt)) -> NodeID);
	   sc_core::wait(sem->cond);	
	   //printf("sc_core::wait(sem->cond) --- ---%d \n\n", ((lwip_context*)(sem->ctxt)) -> NodeID);
	}

        (  sim_ctxt.get_os_ctxt( sc_core::sc_get_current_process_handle() ) ) ->os_port->postWait(taskID);
    
  }
  //printf("sys_arch_sem_wait after %d ... ...%d %d\n",timeout, ((lwip_context*)(sem->ctxt)) -> NodeID, taskID);
  sem->c--;
  //printf("Return timeout %ld\n", time_needed);
  return time_needed;
}
void sys_sem_signal(struct sys_sem **s)
{

  struct sys_sem *sem;
  LWIP_ASSERT("invalid sem", (s != NULL) && (*s != NULL));
  sem = *s;

  int taskID = sim_ctxt.get_task_id(sc_core::sc_get_current_process_handle());
  //( (lwip_context*)(sem->ctxt) ) -> getOSModelTaskID( sc_core::sc_get_current_process_handle());
  sem->blocking_task_id = taskID;

  sem->c++;
  if (sem->c > 1) {
    sem->c = 1;
  }
  //( (lwip_context*)(sem->ctxt) ) -> os_port->preNotify(taskID, sem->blocked_task_id);
  sem->cond.notify(sc_core::SC_ZERO_TIME);
  //( (lwip_context*)(sem->ctxt) ) -> os_port->postNotify(taskID, sem->blocked_task_id);

}


static void
sys_sem_free_internal(struct sys_sem *sem)
{
  //pthread_cond_destroy(&(sem->cond));
  //pthread_mutex_destroy(&(sem->mutex));
  //delete sem;
  sem->free=1;
}
void
sys_sem_free(struct sys_sem **sem)
{
  if ((sem != NULL) && (*sem != SYS_SEM_NULL)) {
    SYS_STATS_DEC(sem.used);
    sys_sem_free_internal(*sem);
  }
}
#endif /* !NO_SYS */
u32_t
sys_now(void)
{
  //struct timeval tv;
  //double sec;
  sc_dt::uint64 msec;
  //gettimeofday(&tv, NULL);
  //sec = (u32_t)(tv.tv_sec - starttime.tv_sec);
  msec = (sc_core::sc_time_stamp().value()/1000000000) - starttime;

  return (u32_t)msec;
}
u32_t
sys_jiffies(void)
{
  //struct timeval tv;
  //double sec;
  sc_dt::uint64 msec;
  //gettimeofday(&tv, NULL);
  //sec = (u32_t)(tv.tv_sec - starttime.tv_sec);
  msec = (sc_core::sc_time_stamp().value()/1000000000) - starttime;

  return (u32_t)(msec*1000000);
}

void
sys_init(void)
{
  starttime =(sc_dt::uint64) (sc_core::sc_time_stamp().value()/1000000000);
}

sys_prot_t
sys_arch_protect(void)
{
    /* Note that for the UNIX port, we are using a lightweight mutex, and our
     * own counter (which is locked by the mutex). The return code is not actually
     * used. */
    if (lwprot_thread != sc_core::sc_get_current_process_handle())
    {
        /* We are locking the mutex where it has not been locked before *
        * or is being locked by another thread */
        lwprot_mutex.lock();
        lwprot_thread = sc_core::sc_get_current_process_handle();
        lwprot_count = 1;
    }
    else
        /* It is already locked by THIS thread */
        lwprot_count++;
    return 0;
}
void
sys_arch_unprotect(sys_prot_t pval)
{
    LWIP_UNUSED_ARG(pval);
    if (lwprot_thread == sc_core::sc_get_current_process_handle())
    {
        if (--lwprot_count == 0)
        {
            lwprot_thread = SC_DEAD_PROCESS;
            lwprot_mutex.unlock();
        }
    }
}
/*-----------------------------------------------------------------------------------*/


