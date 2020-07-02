#include <yaos/types.h>
#include <yaos/init.h>
#include <yaos/percpu.h>
#include <errno.h>
#include <yaoscall/malloc.h>
#include <yaos/sched.h>
#include <yaos/irq.h>
#include <yaos/assert.h>
#include <yaos/tasklet.h>
#include <yaos/time.h>
#include <yaos/clockevent.h>
#define NR_TIMER_BUF 64 
DEFINE_PER_CPU(struct clock_event_device, the_cev);

extern u64 msec_count;
typedef void (*timer_callback) (u64 uptime, void *param);
struct timeout_callback;
struct timeout_callback {
    //first three must be same as poll_cb
    struct timeout_callback *pnext;
    poll_handle_t handle;
    struct thread_struct *thread;
    void *data;
    void *param;
    u64 deadline;
    timer_callback callback;
    int count;
    
};
struct timer_manager {
    struct timeout_callback *pfree;
    struct timeout_callback *ptimer;
    struct timeout_callback timerbuf[NR_TIMER_BUF];
};
static DEFINE_PER_CPU(struct timer_manager,ntime);//纳秒
static int err_poll(struct poll_cb *p_poll)
{
    panic("err_poll:%lx\n", p_poll);
    return 0;
}
static void free_poll(struct timeout_callback *p)
{
    p->handle = err_poll;
    __thread struct timer_manager *ptime = this_cpu_ptr(&ntime);
    p->pnext = ptime->pfree;
    ptime->pfree = p;

}
__used static int cnt=0;
static int timeout_poll(struct poll_cb *p_poll)
{
//    if (++cnt%1000==0)printk("timeout_poll:%p",p_poll);
    struct timeout_callback *p = (struct timeout_callback *)p_poll;
    (*p->callback)(p->deadline, p->param);
    free_poll(p);
    return 0;
}
static void add_timer(u64 nsec, struct timeout_callback *p)
{
    p->count = 1;
    __thread struct timer_manager *ptime = this_cpu_ptr(&ntime);
    unsigned long flag = local_irq_save();

    barrier();
    if (!ptime->ptimer || p->deadline < ptime->ptimer->deadline) {
        p->pnext = ptime->ptimer;
        ptime->ptimer = p;
        struct clock_event_device *levt = this_cpu_ptr(&the_cev);
        levt->set_next_event(nsec,levt);
    }
    else {
        struct timeout_callback *ptr = ptime->ptimer;

        while (ptr->pnext && ptr->pnext->deadline < p->deadline) {
            ptr = ptr->pnext;
        }
        p->pnext = ptr->pnext;
        ptr->pnext = p;

    }
    barrier();
    local_irq_restore(flag);

}
static inline  bool is_poll(struct timeout_callback *p)
{
    return p->thread != 0;
}
//call by softirq
static void timer_cb(struct timeout_callback *p)
{
   //printk("tttttttttttttttttttttttimer_cb:%lx\n",p->thread);
   ASSERT(--p->count==0);
   if (!is_poll(p)) {
       (*p->callback)(p->deadline, p->param);
       free_poll(p);

   } else {
       thread_add_poll(p,POLL_LVL_TIMER);

   }
   
}
int set_timeout_nsec_cb(u64 nsec, timer_callback pfunc, void *param)
{
     __thread struct timer_manager *ptime = this_cpu_ptr(&ntime);
    struct timeout_callback *p = ptime->pfree;
    if (!p) {
        p = yaos_malloc(sizeof(*p));
        if (!p)
            return ENOMEM;
    }
    else {
        ptime->pfree = p->pnext;
    }
    u64 thedeadline = nsec + hrt_uptime();
    //printk("deadline:%lx\n",thedeadline);
    p->deadline = thedeadline;
    p->callback = pfunc;
    p->param = param;
    p->thread = 0;
    p->handle = 0;

    add_timer(nsec, p);
    return 0;

}
int set_timeout_nsec_poll(u64 nsec, timer_callback pfunc, void *param)
{
    __thread struct timer_manager *ptime = this_cpu_ptr(&ntime);
    struct timeout_callback *p = ptime->pfree;
    if (!p) {
        p = yaos_malloc(sizeof(*p));
        if (!p)
            return ENOMEM;
    }
    else {
        ptime->pfree = p->pnext;
    }
    u64 thedeadline = nsec + hrt_uptime();
    //printk("deadline:%lx\n",thedeadline);
    p->deadline = thedeadline;
    p->callback = pfunc;
    p->param = param;
    p->thread = current;
    p->handle = timeout_poll;
    add_timer(nsec, p);
    return 0;
}

void run_local_timers(void)
{
    raise_softirq(TIMER_SOFTIRQ);
}
void run_hrt_timers(void)
{
    raise_softirq(HRTIMER_SOFTIRQ);
}
void run_timer_softirq(struct softirq_action *paction)
{
    return;
}
void run_hrtimer_softirq(struct softirq_action *paction)
{
    u64 uptime = hrt_uptime();
    struct timer_manager *ptime = this_cpu_ptr(&ntime);
    //if(ptime->ptimer)printk("uptime:%ld,deadline:%ld\n",uptime,ptime->ptimer->deadline);
    //else printk("null ptime\n");
    if (!ptime->ptimer || uptime < ptime->ptimer->deadline) {
        return;
    }
    barrier();

    //unsigned flag = local_save_flags();
    local_irq_disable();
    struct timeout_callback *p = ptime->ptimer;
    struct timeout_callback *ptr = p;
    void *paramarr[20];
    int callnr = 20;
    ASSERT(p);
    paramarr[--callnr] = ptr;
    while (ptr->pnext && uptime >= ptr->pnext->deadline && callnr > 0) {
        ptr = ptr->pnext;
        paramarr[--callnr] = ptr;
    }
    ptime->ptimer = ptr->pnext;
    
    if (ptime->ptimer) {
        struct clock_event_device *levt = this_cpu_ptr(&the_cev);
        ulong nsec = ptime->ptimer->deadline - uptime;
        levt->set_next_event(nsec,levt);

    }
    
    local_irq_enable();
    for (int i = 19; i >= callnr; i--) {
        timer_cb(paramarr[i]);
    }
    //local_irq_restore(flag);
    //if (callnr == 0)
    //    run_hrtimer_softirq(paction);
}

time64_t mktime64(const unsigned int year0, const unsigned int mon0,
                  const unsigned int day, const unsigned int hour,
                  const unsigned int min, const unsigned int sec)
{
    unsigned int mon = mon0, year = year0;

    /* 1..12 -> 11,12,1..10 */
    if (0 >= (int)(mon -= 2)) {
        mon += 12;              /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return ((((time64_t)
              (year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) + year * 365 - 719499) * 24 + hour	/* now have hours */
            ) * 60 + min        /* now have minutes */
        ) * 60 + sec;           /* finally seconds */
}
__init int static init_hrtimer(bool isbp)
{
    struct timer_manager *ptime = this_cpu_ptr(&ntime);

    for (int i = 0; i < NR_TIMER_BUF - 1; i++) {
        ptime->timerbuf[i].pnext = &ptime->timerbuf[i + 1];
    }
    ptime->timerbuf[NR_TIMER_BUF - 1].pnext = NULL;

    ptime->pfree = &ptime->timerbuf[0];
    ptime->ptimer = NULL;
    if (isbp) {
        open_softirq(HRTIMER_SOFTIRQ, run_hrtimer_softirq);
        open_softirq(TIMER_SOFTIRQ, run_timer_softirq);
    }
    return 0;
}

early_initcall(init_hrtimer);

