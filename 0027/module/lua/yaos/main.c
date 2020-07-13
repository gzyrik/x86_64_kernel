#include <types.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>

#include <yaos/errno.h>
#include <yaos/printk.h>
#include <yaos/lua_module.h>
int yaos_lua_inject_split(lua_State *L);
//lua_poll_fire(data)
int  lua_poll_fire(lua_State *L)
{
    printk("lua_poll_fire\n");
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, -2);
    lua_call(L, 1, 0);
    return 0;
}
//new Promise(lua_poll_fn)
//lua_poll_fn(resolve,reject)
int lua_poll_fn(lua_State *L)
{
    
    struct lua_poll *p  = lightu2v(lua_touserdata(L,lua_upvalueindex(1)));
    lua_pushlightuserdata(L, v2lightu(p));
    void (*create)(struct lua_poll *p) = lightu2v(lua_touserdata(L,lua_upvalueindex(2)));
    printk("lua_poll_fn: lua_poll:%lx, create_fn:%lx\n",p, create);
    lua_pushvalue(L, -3);//resolve
    lua_pushvalue(L, -3);//reject
    lua_pushcclosure(L, lua_poll_fire, 3);
    p->callback = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p->callback);
    printk("callback:%d,%d\n",p->callback,lua_type(L, -1));
    lua_pop(L, 1);
    if (create) (*create)(p); 
    return 0;
}
int lua_poll_new_promise(lua_State *L, struct lua_poll *p, void *cb)
{
    printk("lua_poll_new_promise:%p,%p,%p\n",L,p,cb);
    lua_pushcfunction(L, lua_new_promise);
    lua_pushlightuserdata(L, v2lightu(p));
    lua_pushlightuserdata(L, v2lightu(cb));
    lua_pushcclosure(L, lua_poll_fn, 2);
    lua_call(L, 1, 1);

    //lua_new_promise(L);
    return 1;
}
static int test(lua_State *L)
{
    //lua_pushcfunction(L, test_close2);
    return 0;
    
}
extern int yaos_lua_inject_time(lua_State *L);
extern int yaos_lua_inject_promise(lua_State *L);
static int yaos_lua_version(lua_State *L)
{
    lua_pushstring(L,"0.1.2");
    return 1;
}
const lua_file_t * find_lua_file(char *name)
{
    lua_file_t * p = _lua_file_data_start;
    while(p<_lua_file_data_end) {
        //printk("p:%lx,p->name:%lx\n",p,p->name);
        if (p->name == name || strcmp(p->name,name)==0) return p;
        p++;
    }
    return NULL;
}
int load_lua_file(lua_State *L, char *name)
{
    const lua_file_t *p = find_lua_file(name);
    if (!p) return ESRCH;
    //printk("lua_file %016lx,L:%lx,p->name:%lx,name:%lx,%s,buf:%lx,size:%x\n",p,L,p->name,name,name,p->buf,p->size);
    luaL_loadbuffer(L,p->buf,p->size,p->name);
    //printk("luaL_loadbuffer\n");
    return 0;
}
int do_lua_file(lua_State *L, char *name)
{
    return load_lua_file(L, name) || lua_pcall(L, 0, LUA_MULTRET, 0);
}

static int yaos_lua_do_file(lua_State *L)
{
    size_t len;
    char *p = (uchar *) luaL_checklstring(L, 1, &len);
    if (len>=64) return luaL_error(L, "file name too long, max len 63");
    //printk("yaos_lua_do_file len:%d,%s\n",len,p);
    return do_lua_file(L, p); 
}
static int yaos_lua_say(lua_State *L)
{
    ssize_t vga_write(void *p, size_t s);

    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "yaos.say expecting one argument");
    uchar *p;
    size_t len;
    if (lua_type(L,1) == LUA_TNIL) {
        printk("yaos.say nil:%d,%d,%d\n", lua_type(L, -1),lua_type(L, -2),lua_gettop(L));
        return 0;
    }
    p = (uchar *) luaL_checklstring(L, 1, &len);
    size_t w = vga_write(p, len);
    //printk("cpu_base:%016lx\n",this_cpu_read(this_cpu_off));
    lua_pushnumber(L, (lua_Number) w);
    return 1;
}
__used static int yaos_main(lua_State * L, ulong event)
{
    if (event == LUA_MOD_INSTALL) {
        lua_createtable(L, 0 /* narr */, 99 /* nrec */);    /*yaos.* */
        lua_pushcfunction(L, yaos_lua_version);
        lua_setfield(L, -2, "version");
        lua_pushcfunction(L, yaos_lua_say);
        lua_setfield(L, -2, "say");
        lua_pushcfunction(L, yaos_lua_do_file);
        lua_setfield(L, -2, "do_file");
        lua_pushcfunction(L, test);
        lua_setfield(L, -2, "test");
        yaos_lua_inject_time(L);
        yaos_lua_inject_promise(L);
        yaos_lua_inject_split(L);
        lua_setglobal(L, "yaos");


    }
    return LUA_MOD_ERR_OK;

}
DECLARE_LUA_MODULE(yaos, yaos_main);
