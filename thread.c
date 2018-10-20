#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <nuklear.h>

#include "fns.h"

int mainstacksize = 128*1024;

void
usage(void)
{
	fprint(2, "usage: %s\n", argv0);
	threadexitsall("usage");
}

void
kbdthread(void *v)
{
	int fd;
	long n;
	Channel *c;
	Ioproc *io;
	IOchunk chk;

	c = v;
	io = ioproc();

	if((fd = ioopen(io, "/dev/kbd", OREAD)) < 0)
		sysfatal("open: %r");

	for(;;){
		chk.addr = malloc(64);
		if(chk.addr == nil)
			sysfatal("malloc: %r");

		n = ioread(io, fd, chk.addr, 64);
		if(n < 0)
			sysfatal("read: %r");

		chk.len = n;

		send(c, &chk);
	}
}

void
timerthread(void *v)
{
	Channel *c;
	Ioproc *io;
	vlong t;

	static long sleepms = 100;

	c = v;
	io = ioproc();
	for(;;){
		iosleep(io, sleepms);
		t = nsec();
		send(c, &t);
	}
}

void
threadmain(int argc, char *argv[])
{
	vlong t;
	IOchunk kbdchk;
	Channel *kbd, *timer;
	Mouse m;
	Mousectl *mctl;

	struct nk_user_font nkfont;
	struct nk_context sctx;
	struct nk_context *ctx = &sctx;

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(initdraw(nil, nil, "a nuklear demo") < 0)
		sysfatal("initdraw: %r");

	kbd = chancreate(sizeof(IOchunk), 0);
	timer = chancreate(sizeof(vlong), 0);

	threadcreate(kbdthread, kbd, 8192);
	threadcreate(timerthread, timer, 8192);

	mctl = initmouse(nil, screen);

	/* create nuklear font from default libdraw font */
	nk_plan9_makefont(&nkfont, font);

	/* initialize nuklear */
	if(!nk_init_default(ctx, &nkfont))
		sysfatal("nk_init: %r");

	enum { ATIMER, AKBD, AMOUSE, ARESIZE, AEND };
	Alt alts[] = {
	[ATIMER]	{ timer,			&t,			CHANRCV },
	[AKBD]		{ kbd,				&kbdchk,	CHANRCV },
	[AMOUSE]	{ mctl->c,			&m,			CHANRCV },
	[ARESIZE]	{ mctl->resizec,	nil,		CHANRCV },
	[AEND]		{ nil,				nil,		CHANEND },
	};

	for(;;){
		nk_input_begin(ctx);
		switch(alt(alts)){
		case ATIMER:
			break;
		case AKBD:
			nk_plan9_handle_kbd(ctx, kbdchk.addr, kbdchk.len);
			free(kbdchk.addr);
			kbdchk.addr = nil;
			break;
		case AMOUSE:
			nk_plan9_handle_mouse(ctx, m, screen->r.min);
			break;
		case ARESIZE:
			if(getwindow(display, Refnone) < 0)
				sysfatal("getwindow: %r");
		}
		nk_input_end(ctx);

		/* handle usual plan 9 exit key */
		if(nk_input_is_key_down(&ctx->input, NK_KEY_DEL))
			threadexitsall(nil);

		overview(ctx);
		calculator(ctx);

		/* render everything */
		draw(screen, screen->r, display->black, nil, ZP);
		nk_plan9_render(ctx, screen);
		flushimage(display, 1);
	}

	threadexitsall(nil);
}
