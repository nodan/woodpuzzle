/*
  Solve a woodpuzzle by moving pieces.
  GPL2
  (c) Kai Tomerius, 2016-2017
 */

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// size of the puzzle
#define H 5
#define W 4

// number of pieces
#define N 10

// upper bound of possible positions
#define S (((N+1)*(N+1))<<(2*(N-1)))

// search criteria for tree search
#define X -1
#define MIN -1
#define EQ 0
#define MAX 1

// bitmasked for hashtable scan
#define ILLEGAL  0x00
#define LEGAL    0x01
#define REACHED  0x02
#define EXPANDED 0x04
#define FOUND    0x40
#define FINAL    0x80

// some type definitions
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned long      uint32;
typedef unsigned long long uint64;

// how search: depth first tree search or hashtable scan
static enum mode { depth_first, scan } mode = depth_first;

// one piece of the puzzle
class piece {
    friend class puzzle;

private:
    // dimensions
    int x, y, w, h;

    // success criteria
    int tx, ty, match;

    // move piece
    int move(class puzzle& board, int dx, int dy);

    // evaluate success criteria
    int nsolved() {
        switch (match) {
        case MIN:
            return (tx!=X && x<tx) || (ty!=X && y<ty);
        case MAX:
            return (tx!=X && x>tx) || (ty!=X && y>ty);
        }

        return (tx>=0 && x!=tx) || (ty>=0 && y!=ty);
    }

public:
    piece(int x=0, int y=0, int w=1, int h=1, int tx=X, int ty=X, int match=EQ)
        : x(x), y(y), w(w), h(h), tx(tx), ty(ty), match(match) {
    }

    // check if pieces are at identical position and size
    int operator==(const piece& p) {
        return this ? x==p.x && y==p.y && w==p.w && h==p.h : 0;
    }

    // check if the piece overlaps the given coordinates
    int overlaps(int nx, int ny) {
        return nx>=x && nx<x+w && ny>=y && ny<y+h;
    }

    // move piece and calculate the hashvalue of the resulting position
    int move(class puzzle& board, int dx, int dy, uint32& hashvalue);

    // determine if this piece solves the puzzle
    int solved(class puzzle& board);
};

// the puzzle
class puzzle {
private:
    piece* board[N];
    int pieces;
    int illegal;
    int final;
    int tries;
    int recursions;

    static uint8 hashtable[S];
    static uint32 backtrace[S];
    static uint32 sum;
    static uint64 moves;

    // check if the given position has been searched before
    int fails(uint8 depth, uint32 hashvalue) {
        if (hashtable[hashvalue]<depth) {
            hashtable[hashvalue] = depth;
            return 0;
        }

        return 1;
    }

    // solve the puzzle by searching to the given depth
    int solve(uint8 depth, uint32 from=0) {
        if (depth) {
            recursions++;
            for (int i=0; i<pieces; i++) {
                for (int dy=-1; dy<2; dy++) {
                    for (int dx=-1; dx<2; dx++) {
                        if ((dx && !dy) || (!dx && dy)) {
                            uint32 hashvalue;
                            if (board[i]->move(*this, dx, dy, hashvalue)) {
                                if (mode==depth_first) {
                                    // recursively search depth first
                                    tries++;
                                    if (board[i]->solved(*this) || (!fails(depth, hashvalue) && solve(depth-1))) {
                                        if (depth==1) {
                                            std::cout << moves << " moves" << std::endl;
                                        }

                                        print_nice();
                                        board[i]->move(*this, -dx, -dy);

                                        return 1;
                                    }
                                } else {
                                    // scan hashtable and keep track how a position has been reached
                                    if (!(hashtable[hashvalue] & REACHED)) {
                                        hashtable[hashvalue] |= REACHED;
                                        backtrace[hashvalue] = from;
                                    }
                                }

                                // undo move
                                board[i]->move(*this, -dx, -dy);
                            }
                        }
                    }
                }
            }
        }

        return 0;
    }

public:
    // setup the start position of the puzzle
    puzzle()
        : pieces(0), illegal(0), final(0), tries(0), recursions(0) {
        memset(board, 0, sizeof(board));

        /*
          #### ######### ####
          #### ######### ####
          #### ######### ####
          #### ######### ####
          #### ######### ####
          #### ######### ####
          #### ######### ####

               #########
               #########
               #########

          #### #### #### ####
          #### #### #### ####
          #### #### #### ####
          ####           ####
          #### #### #### ####
          #### #### #### ####
          #### #### #### ####
        */
        add(1, 0, 2, 2, 1, 3);
        add(1, 2, 2, 1);
        add(0, 0, 1, 2);
        add(3, 0, 1, 2);
        add(0, 3, 1, 2);
        add(3, 3, 1, 2);
        add(1, 3);
        add(2, 3);
        add(1, 4);
        add(2, 4);
    }

    // try to solve the puzzle by scanning the hashtable for all legal positions
    void scan_hashtable(int all=0) {
        int legal = 0;
        int final = 0;
        int total = 0;
        uint32 start = calculate_hashvalue();

        for (int i=0; i<S; i++) {
            puzzle p(i, pieces, piece(1, 3, 2, 2));
            hashtable[i] = p ? (legal++, (p.final ? (final++, LEGAL|FINAL) : LEGAL)) : ILLEGAL;
            if (i%10000==0) {
                std::cout << "\r" << i/(S/100) << "%" << std::flush;
            }
        }

        std::cout
            << std::endl
            << legal << " legal positions" << std::endl
            << final << " final positions" << std::endl;

        mode = scan;
        hashtable[start] |= REACHED;

        int solved = 0;
        while (1) {
            int expand = 0;
            for (int i=0; i<S; i++) {
                if (hashtable[i] & LEGAL) {
                    if ((hashtable[i] & (REACHED|FINAL|FOUND))==(REACHED|FINAL)) {
                        hashtable[i] |= FOUND;
                        solved++;

                        if (!all) {
                            int n = 0;
                            while (i) {
                                std::cout << ++n << " " << std::hex << "# 0x" << i << std::dec << std::endl;
                                puzzle p(i, pieces);
                                p.print_nice();

                                i = backtrace[i];
                            }
                            break;
                        } else {
                            std::cout << "\rsolution " << std::hex << i << std::dec << " found" << std::endl;
                            puzzle p(i, pieces);
                            p.print_nice();
                        }
                    }

                    if ((hashtable[i] & (REACHED|FINAL|EXPANDED))==REACHED) {
                        puzzle p(i, pieces);
                        p.solve(1, i);
                        hashtable[i] |= EXPANDED;
                        expand++;
                    }
                }
            }

            if (solved && !all) {
                break;
            }

            if (!expand) {
                break;
            }

            total += expand;
            std::cout << "\r" << total << " expansions" << std::flush;
        }

        if (all) {
            std::cout << solved << " solutions found" << std::endl;
        }
}

    // setup a position from its hashvalue
    puzzle(uint32 hashvalue, int pieces, const piece& solution=piece())
        : pieces(0), illegal(0), final(0), tries(0), recursions(0) {
        memset(board, 0, sizeof(board));
        uint8 grid[H*W] = { 0 };
        uint32 e = hashvalue % ((pieces+1)*(pieces+1));
        hashvalue /= (pieces+1)*(pieces+1);

        uint32 i = 0, n = 0, p = 0, r = pieces-1, s = 0;
        while (p<H*W) {
            if (i==e/(pieces+1)%(pieces+1)) {
                n++;
                p++;
                e *= pieces+1;
            } else {
                int h, w;
                uint32 v = (r ? hashvalue>>(2*--r) : sum-s) & 0x03;
                switch (v) {
                case 0: h = 0; w = 0; break;
                case 1: h = 0; w = 1; break;
                case 2: h = 1; w = 0; break;
                case 3: h = 1; w = 1; break;
                }

                if (p+h*W+w>=H*W || p/W!=(p+w)/W) {
                    illegal++;
                } else {
                    grid[p+w] = 5;
                    grid[p+h*W] = 5;
                    grid[p+h*W+w] = 5;
                    grid[p] = v+1;
                }

                s += v;
                i++;
            }

            while (p<H*W && grid[p]) {
                p++;
            }
        }

        if (n!=2 || i!=pieces || s!=sum) {
            illegal++;
        }

        for (int y=H; y--;) {
            for (int x=W; x--;) {
                if (grid[y*W+x]) {
                    if (grid[y*W+x]<5) {
                        if (add(x, y, 1+(grid[y*W+x]-1)%2, 1+(grid[y*W+x]-1)/2)==solution) {
                            final++;
                        }
                    }
                }
            }
        }
    }

    ~puzzle() {
        while (pieces--) {
            delete board[pieces];
        }
    }

    // add a piece to the puzzle
    piece& add(int x=0, int y=0, int w=1, int h=1, int tx=X, int ty=X, int match=EQ) {
        if (pieces<N) {
            return *(board[pieces++] = new piece(x, y, w, h, tx, ty, match));
        } else {
            illegal++;
            return *((piece*) NULL);
        }
    }

    int operator()() {
        return pieces;
    }

    piece* operator[](int i) {
        return board[i];
    }

    int operator++() {
        return ++moves;
    }

    operator void* () {
        return illegal ? NULL : this;
    }

    // enumerate all possible positions
    // 9*2 bits for 10 pieces + 7 bits for empty spaces
    uint32 calculate_hashvalue() {
        uint8 grid[H*W] = { 0 };
        for (int i=0; i<pieces; i++) {
            for (int dy=board[i]->h; dy--; ) {
                for (int dx=board[i]->w; dx--; ) {
                    grid[(board[i]->y+dy)*W+board[i]->x+dx] =
                        dy || dx ? 5 : 2*(board[i]->h-1)+board[i]->w;
                }
            }
        }

        uint32 e = 0, i = 0, hashvalue = 0;
        sum = 0;
        for (int y=0; y<H; y++) {
            for (int x=0; x<W; x++) {
                if (grid[y*W+x]) {
                    if (grid[y*W+x]<5) {
                        hashvalue = (hashvalue<<2) | (grid[y*W+x]-1);
                        sum += grid[y*W+x]-1;
                        i++;
                    }
                } else {
                    e = e*(pieces+1)+i;
                }
            }
        }

        return ((hashvalue>>2)*(pieces+1)*(pieces+1))+e;
    }

    // print a position
    void print() {
        std::cout << std::endl;
        for (int y=H; y--;) {
            for (int x=0; x<W; x++) {
                char c = '.';
                for (int i=0; i<pieces; i++) {
                    if (board[i]->overlaps(x, y)) {
                        c = 'A'+i;
                    }
                }
                std::cout << c;
            }
            std::cout << std::endl;
        }
    }

    // print a position nicely
    void print_nice() {
        char grid[H*W] = { ' ' };
        memset(grid, ' ', sizeof(grid));
        for (int y=H; y--;) {
            for (int x=0; x<W; x++) {
                char c = '.';
                for (int i=0; i<pieces; i++) {
                    if (board[i]->overlaps(x, y)) {
                        grid[y*W+x] = 'A'+i;
                    }
                }
            }
        }

        std::cout << std::endl;
        for (int y=H; y--;) {
            for (int i=0; i<3; i++) {
                std::cout << ' ';
                for (int x=0; x<W; x++) {
                    for (int j=0; j<4; j++) {
                        std::cout << (grid[y*W+x]!=' ' ? '#' : ' ');
                    }
                    std::cout << (x<W-1 && grid[y*W+x]!= ' ' && grid[y*W+x]==grid[y*W+x+1] ? '#' : ' ');
                }
                std::cout << std::endl;
            }

            std::cout << ' ';
            for (int x=0; x<W; x++) {
                for (int j=0; j<4; j++) {
                    std::cout << (y && grid[y*W+x]!=' ' && grid[y*W+x]==grid[(y-1)*W+x] ? '#' : ' ');
                }
                std::cout << (x<W-1 && grid[y*W+x]!=' ' && grid[y*W+x]==grid[y*W+x+1] && y && grid[y*W+x]==grid[(y-1)*W+x] ? '#' : ' ');
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    // iteratively deepening depth-first search
    void solve() {
        struct timeval t0;
        gettimeofday(&t0, NULL);
        for (uint8 depth=1; depth; depth++) {
            struct timeval t;
            gettimeofday(&t, NULL);

            int n = 0;
            for (int i=0; i<((N+1)*(N+1))<<(2*(N-1)); i++) {
                if (hashtable[i]) {
                    n++;
                }
            }

            std::cout << " depth " << (int) depth << " " << t.tv_sec - t0.tv_sec << "s #" << n << " %" << recursions/(tries/100+1) << std::endl;
            recursions = tries = 0;

            if (solve(depth)) {
                print_nice();
                break;
            }
        }

        std::cout << std::endl;
    }
};

// move piece if possible
int piece::move(class puzzle& board, int dx, int dy) {
    if (x+dx<0 || x+w+dx>W || y+dy<0 || y+h+dy>H) {
        return 0;
    }

    for (int i=0; i<board(); i++) {
        if (board[i]!=this) {
            if ((board[i]->overlaps(x+dx, y+dy) || board[i]->overlaps(x+w+dx-1, y+dy) ||
                 board[i]->overlaps(x+dx, y+h+dy-1) || board[i]->overlaps(x+w+dx-1, y+h+dy-1))) {
                return 0;
            }
        }
    }

    x += dx;
    y += dy;

    if (mode==depth_first) {
        int moves = ++board;
        if (moves%1000==0) {
            std::cout << "\r" << moves << " moves" << std::flush;
        }
    }

    return 1;
}

// move piece and calculate the hashvalue of the resulting position
int piece::move(class puzzle& board, int dx, int dy, uint32& hashvalue) {
    if (move(board, dx, dy)) {
        hashvalue = board.calculate_hashvalue();
        return 1;
    }

    return 0;
}

// determine if this piece solves the puzzle
int piece::solved(class puzzle& board) {
    if (tx==X && ty==X) {
        return false;
    }

    for (int i=0; i<board(); i++) {
        if (board[i]->nsolved()) {
            return false;
        }
    }

    std::cout << std::endl;
    for (int i=0; i<board(); i++) {
        std::cout << i << ":" << board[i]->x << " " << board[i]->y << " " << board[i]->tx << " " << board[i]->ty << " " << board[i]->match << std::endl;
    }

    return true;
}

uint64 puzzle::moves = 0;
uint8 puzzle::hashtable[S] = { 0 };
uint32 puzzle::backtrace[S] = { 0 };
uint32 puzzle::sum = 0;

int main(int argc, const char** argv) {
    puzzle p;

    if (argc>1 && !strcmp(argv[1], "--tree")) {
        // depth-first search
        p.solve();
    } else if (argc>1 && !strcmp(argv[1], "--all")) {
        // scan hashtable for all solutions
        p.scan_hashtable(1);
    } else if (argc==1) {
        // scan hashtable for one solution
        p.scan_hashtable(0);
    } else {
        std::cout <<"usage: " << argv[0] << " [--all|--tree]" << std::endl;
    }

    return 0;
}
