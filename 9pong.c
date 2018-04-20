/*
  * 9pong.c: pong for plan 9
  * sean caron (scaron@umich.edu)
*/

/*
  * The paradigm here is that we must always include u.h
  * first and libc.h second, and anything else after that.
*/

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <cursor.h>
#include <event.h>

int main(void);
void eresized(int new);
void redraw(Point l, Point r, Point b, Point ol, Point or, Point ob);
Point updateball(Point b, Point pr, Point pl);

/* score and ball motion step in x and y are globals to make things easier */
int stepx = 5;
int stepy = 5;

int score = 0;
int o_score = 0;

int main(void) {
    Point pleft, pright, ball, o_pleft, o_pright, o_ball;
    int key, fd;
    char *buttons[] = { "quit", 0 };
    Menu menu = { buttons };
    Mouse m;
    Event e;
    ulong timer, evtype;

    initdraw(nil, nil, "9pong");

    /*
     * initialize event handler for mouse events only. we
     * must call this subsequent to initdraw().
    */

    einit(Emouse|Ekeyboard);

    /* we need to call this resize handler at least once */
    eresized(0);

    /*
     * the timer is used to update the display in lieu of a real
     * event occurring. it also sets the speed of the game. from
     * empirical evidence, 30 is fairly difficult, 35 is reasonable
     * and 40 starts to get boring.
    */

    timer = etimer(0,35);

    /*
     * initdraw() sets up some global variables,
     *
     * Display *display
     * Image *screen
     * Screen *_screen
     * Font *font
     *
     * ZP is a constant point (0,0).
    */

    /* resize the current window. */
    fd = open("/dev/wctl", OWRITE);

    fprint(fd, "resize -dx 300 -dy 300");

    close(fd);

    getwindow(display, Refnone);

    /*
     * initialize some variables. the paddle starts near the middle of
     * the screen and is 45 pixels in length. the ball starts at a random
     * position near the start of the screen.
    */

    pleft.x = screen->r.min.x + (screen->r.max.x - screen->r.min.x)/2;
    pleft.y = screen->r.max.y - 10;
    pright.x = pleft.x + 45;
    pright.y = pleft.y;

    ball.x = screen->r.min.x + nrand(screen->r.max.x - screen->r.min.x);
    ball.y = screen->r.min.y + 50;

    srand(time(0));

    /* main program loop */
    while (1) {
        evtype = event(&e);
	
        if (evtype == Emouse) {
            m = e.mouse;

            /* if left mouse button is clicked, show the menu */
            if (m.buttons == 4) {
			
                /* quit */
                if (emenuhit(3, &m, &menu) == 0) {
                    closedisplay(display);
                    exits(0);
                }
            }
        }

        /* handle keyboard */
        else if (evtype == Ekeyboard) {
            key = e.kbdc;

            /* left arrow key */
            if (key == 61457) {
                pleft.x = pleft.x - 20;
                pright.x = pright.x - 20;
            }

            /* right arrow key */
            if (key == 61458) {
                pleft.x = pleft.x + 20;
                pright.x = pright.x + 20;
            }

            redraw(pleft, pright, ball, o_pleft, o_pright, o_ball);

            o_pright = pright;
            o_pleft = pleft;
            o_ball = ball;

            ball = updateball(ball, pleft, pright);
        }

        /* otherwise just use the faux timer event to keep things updated */
        else if (evtype == timer) {
            redraw(pleft, pright, ball, o_pleft, o_pright, o_ball);

            o_pright = pright;
            o_pleft = pleft;

            o_ball = ball;
            ball = updateball(ball, pleft, pright);
        }
    }	
}

/*
  * by convention we must declare an eresized() function to
  * handle the window resizing event.
*/

void eresized(int new) {
    /* just refresh the window for now. */
    getwindow(display, Refnone);
}

/* draw the paddle, ball, and score */

void redraw(Point l, Point r, Point b, Point ol, Point or, Point ob) {
    Point scorepos;
    char scorebuf[8];

    scorepos.x = screen->r.max.x - 30;
    scorepos.y = screen->r.min.y + 10;

    /* draw the paddle */
    line(screen, ol, or, Endsquare, Endsquare, 2, display->white, ZP);
    line(screen, l, r, Endsquare, Endsquare, 2, display->black, ZP);

    /* draw the ball */
    ellipse(screen, ob, 2, 2, 2, display->white, ZP);
    ellipse(screen, b, 2, 2, 2, display->black, ZP);

    /* if the score has changed, get rid of the old score */
    if (score != o_score) {
        sprint(scorebuf, "%d", o_score);
        string(screen, scorepos, display->white, ZP, font, scorebuf);
    }

    /* draw the new score */
    sprint(scorebuf, "%d", score);
    string(screen, scorepos, display->black, ZP, font, scorebuf);

    o_score = score;

    /* we need this even though the manpage draw(2) implies not */
    flushimage(display, 1);
}

/* update the ball position and do collision detection with the paddle */

Point updateball(Point b, Point pl, Point pr) {
    Point msgpos;

    msgpos.x = screen->r.min.x + (screen->r.max.x - screen->r.min.x)/3;
    msgpos.y = screen->r.min.y + (screen->r.max.y - screen->r.min.y)/3;

    /* hit right edge of window */
    if (b.x + stepx > screen->r.max.x) {
        stepx = (-1)*stepx;
    }

    /* hit left edge of window */
    if  (b.x + stepx < screen->r.min.x) {
        stepx = abs(stepx);
    }

    /* hit bottom of window or paddle */
    if (b.y + stepy > screen->r.max.y) {
        /* hit paddle, taking ball radius into account. */
        if (((b.x-2) <= pr.x) && ((b.x+2) >= pl.x)) {
            stepy = (-1)*stepy;

            if (stepx < 0) {
                stepx = (-1)*nrand(6) - 1;
            }

            if (stepx > 0) {
                stepx = nrand(6) + 1;
            }

            score = score + 1;
        }

        /* missed the paddle */
        else {
            string(screen, msgpos, display->black, ZP, font, "game over!");
            flushimage(display,1);

            sleep(2000);
            closedisplay(display);
            exits(0);
        }
    }

    /* hit top of window */
    if (b.y + stepy < screen->r.min.y) {
        stepy = abs(stepy);
    }

    b.x = b.x + stepx;
    b.y = b.y + stepy;

    return b;
}
