#include <u.h>
#include <libc.h>
#include <draw.h>
#include <keyboard.h>
#include <event.h>
#include <nuklear.h>

#include "fns.h"

enum {
	Ekbd	= Ekeyboard<<1,
	Etimer	= Ekbd<<1,
};

static void
mainmenu(struct nk_context *ctx)
{
	if(nk_begin(ctx, "mainmenu", nk_rect(0, 0, Dx(screen->r), 40),
		NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BACKGROUND)){

		nk_menubar_begin(ctx);
		nk_layout_row_static(ctx, 0, 120, 1);
		if(nk_menu_begin_label(ctx, "Menu", NK_TEXT_LEFT, nk_vec2(120, 200))){
			nk_layout_row_dynamic(ctx, 25, 1);

			if(nk_menu_item_label(ctx, "Calculator", NK_TEXT_LEFT))
				if(nk_window_is_hidden(ctx, "Calculator"))
					nk_window_show(ctx, "Calculator", NK_SHOWN);

			if(nk_menu_item_label(ctx, "Overview", NK_TEXT_LEFT))
				if(nk_window_is_hidden(ctx, "Overview"))
					nk_window_show(ctx, "Overview", NK_SHOWN);

			if(nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT))
				exits(nil);

			nk_menu_end(ctx);
		}
		nk_menubar_end(ctx);
	}
	nk_end(ctx);
}

static void
demo(struct nk_context *ctx)
{
	if (nk_begin(ctx, "Demo", nk_rect(425, 180, 200, 230),
		NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
		NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
	{
		enum {EASY, HARD};
		static int op = EASY;
		static int property = 20;
		static char text[128];

		nk_layout_row_static(ctx, 30, 80, 1);
		if (nk_button_label(ctx, "button"))
			print("button pressed\n");
		nk_layout_row_dynamic(ctx, 30, 2);
		if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
		if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
        nk_layout_row_dynamic(ctx, 75, 1);
		nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, text, sizeof(text)-1, nk_filter_default);
	}
	nk_end(ctx);
	if (nk_window_is_hidden(ctx, "Demo")) exits(nil);
}

void
usage(void)
{
	fprint(2, "usage: %s\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int kbdfd;
	Event e;
	struct nk_user_font nkfont;
	struct nk_context sctx;
	struct nk_context *ctx = &sctx;

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	/* open kbd for up/down events */
	if((kbdfd = open("/dev/kbd", OREAD)) < 0)
		sysfatal("open: %r");

	if(initdraw(nil, nil, "a nuklear demo") < 0)
		sysfatal("initdraw: %r");

	/* need mouse, keyboard, and timer */
	einit(Emouse);
	estart(Ekbd, kbdfd, EMAXMSG);
	etimer(Etimer, 100);

	/* create nuklear font from default libdraw font */
	nk_plan9_makefont(&nkfont, font);

	/* initialize nuklear */
	if(!nk_init_default(ctx, &nkfont))
		sysfatal("nk_init: %r");

	static int firstdraw = 1;

	for(;;){
		/* skip initial timer delay */
		if(firstdraw){
			firstdraw = 0;
			goto draw;
		}

		/* begin input handling block */
		nk_input_begin(ctx);

		switch(event(&e)){
		case Emouse:
			nk_plan9_handle_mouse(ctx, e.mouse, screen->r.min);
			break;

		case Ekbd:
			nk_plan9_handle_kbd(ctx, (char*)e.data, e.n);
			break;

		case Etimer:
			break;
		}

		nk_input_end(ctx);
		/* end input handling block */

		/* handle usual plan 9 exit key */
		if(nk_input_is_key_down(&ctx->input, NK_KEY_DEL))
			exits(nil);

draw:

		mainmenu(ctx);

		overview(ctx);
		calculator(ctx);

		/* hide windows at startup */
		static int hideonce = 1;
		if(hideonce){
			nk_window_show(ctx, "Calculator", NK_HIDDEN);
			nk_window_show(ctx, "Overview", NK_HIDDEN);
			hideonce = 0;
		}

		/* render everything */
		draw(screen, screen->r, display->black, nil, ZP);
		nk_plan9_render(ctx, screen);
		flushimage(display, 1);
	}
}

void 
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0) {
		sysfatal("getwindow: %r");
	}
//	nk_plan9_resized(new);
}
