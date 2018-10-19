#include <u.h>
#include <libc.h>
#include <draw.h>
#include <keyboard.h>
#include <event.h>
#include <nuklear.h>

#include "fns.h"

enum {
	Ekbd	= 4,
};

static Image*
getimage(char *file)
{
	int fd;
	Image *i;

	if((fd = open(file, OREAD)) < 0)
		return nil;

	i = readimage(display, fd, 0);
	close(fd);
	return i;
}

static void
mainmenu(struct nk_context *ctx)
{
	if(nk_begin(ctx, "Main", nk_rect(screen->r.min.x, screen->r.min.y, screen->r.max.x, screen->r.min.y+40),
		NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BACKGROUND)){

		nk_menubar_begin(ctx);
		nk_layout_row_static(ctx, 40, 40, 1);
		if(nk_menu_begin_label(ctx, "Main", NK_TEXT_LEFT, nk_vec2(120, 200))){
			nk_layout_row_dynamic(ctx, 25, 1);
			if(nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT))
				exits(nil);
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

static char*
isdown(struct nk_context *ctx, enum nk_keys key)
{
	if(nk_input_is_key_down(&ctx->input, key))
		return "down";
	return "up";
}

static void
keycheck(struct nk_context *ctx)
{
	char buf[128];
	if(nk_begin(ctx, "Key state", nk_rect(20, 20, 200, 200),
		NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_TITLE|
		NK_WINDOW_SCALABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE)){

		nk_layout_row_static(ctx, 0, 90, 1);
		snprint(buf, sizeof(buf), "Shift: %s", isdown(ctx, NK_KEY_SHIFT));
		nk_label(ctx, buf, NK_TEXT_LEFT);
		snprint(buf, sizeof(buf), "Ctrl: %s", isdown(ctx, NK_KEY_CTRL));
		nk_label(ctx, buf, NK_TEXT_LEFT);
		snprint(buf, sizeof(buf), "Tab: %s", isdown(ctx, NK_KEY_TAB));
		nk_label(ctx, buf, NK_TEXT_LEFT);
		snprint(buf, sizeof(buf), "Enter: %s", isdown(ctx, NK_KEY_ENTER));
		nk_label(ctx, buf, NK_TEXT_LEFT);
	}
	nk_end(ctx);
	if (nk_window_is_hidden(ctx, "Key state")) exits(nil);
}

static void
photo(struct nk_context *ctx, Image *img)
{
	struct nk_image nkimg;
	static struct nk_rect r;
	static char title[64];

	nkimg = nk_image_ptr(img);

	snprint(title, sizeof(title), "Image Viewer %.0f,%.0f,%.0f,%.0f",
		r.x, r.y, r.w, r.h);

	if(nk_begin_titled(ctx, "imageviewer", title, nk_rect(425, 10, Dx(img->r)+35, Dy(img->r)+35),
		NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_TITLE|
		NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)){
		r = nk_window_get_bounds(ctx);
		nk_layout_row_dynamic(ctx, 100, 1);
		nk_image(ctx, nkimg);
	}
	nk_end(ctx);
}

static void
chart(struct nk_context *ctx)
{
	int i;
	float id = 0, step = (2*3.141592654f) / 32;

	if(nk_begin(ctx, "Chart", nk_rect(10, 10, 200, 150), NK_WINDOW_TITLE|NK_WINDOW_MINIMIZABLE)){
		nk_layout_row_dynamic(ctx, 100, 1);
		if (nk_chart_begin(ctx, NK_CHART_LINES, 32, -1.0f, 1.0f)) {
			for (i = 0; i < 32; ++i) {
				nk_chart_push(ctx, (float)cos(id));
				id += step;
			}
			nk_chart_end(ctx);
		}
	}
	nk_end(ctx);
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
	int doinput, running, kbdfd;
	Event e;
	struct nk_user_font nkfont;
	struct nk_context sctx;
	struct nk_context *ctx = &sctx;
	Image *scr, *gopher;

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if((kbdfd = open("/dev/kbd", OREAD)) < 0)
		sysfatal("open: %r");

	if(initdraw(nil, nil, "a nuklear demo") < 0)
		sysfatal("initdraw: %r");

	nk_plan9_makefont(&nkfont, font);

	if(!nk_init_default(ctx, &nkfont))
		sysfatal("nk_init: %r");

	gopher = getimage("pkg.bit");

	einit(Emouse);
	estart(Ekbd, kbdfd, EMAXMSG);

	doinput = 0;
	running = 1;
	while(running){
			//scr = display->image;
			scr = screen;
			if(doinput){
				nk_input_begin(ctx);
				//while(ecanread(~0UL)){
					switch(eread(~0UL, &e)){
					case Emouse:
						nk_plan9_handle_mouse(ctx, e.mouse, scr->r.min);
						break;
					case Ekbd:
						nk_plan9_handle_kbd(ctx, (char*)e.data, e.n);
						break;
					}
				//}
				nk_input_end(ctx);
			}
			doinput = 1;

			if(nk_input_is_key_down(&ctx->input, NK_KEY_DEL))
				exits(nil);

			/* GUI */
			mainmenu(ctx);
			//chart(ctx);
			//overview(ctx);
			demo(ctx);
			//keycheck(ctx);
			//calculator(ctx);
			//if(gopher != nil)
			//	photo(ctx, gopher);

			draw(scr, scr->r, display->black, nil, ZP);
			nk_plan9_render(ctx, scr);
			flushimage(display, 1);
	}

	exits(nil);
}

void 
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0) {
		sysfatal("getwindow: %r");
	}
//	nk_plan9_resized(new);
}
