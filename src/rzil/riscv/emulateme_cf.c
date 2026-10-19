/**
 * \file
 * Control flow emulateme for RISC-V.
 *
 * The plain emulateme (src/rzil/emulateme.nostd.c) only exercises `for` loops, `if` without `else`
 * and early returns. This one runs a set of small, self checking algorithms that together cover the
 * remaining C control flow constructs, plus inline assembly that hits the boundaries of the RISC-V
 * branch and jump encodings and jumps between C labels and assembly labels in both directions.
 *
 * Every check writes one letter into `report`: 'A' + index when it passes and '-' when it fails, so
 * after emulating from `_start` up to `emulateme_done`, `ps @ obj.report` shows which part went wrong.
 * Natively (or under qemu-user) the report is printed and the exit status is the number of failures.
 *
 *   A  parse_number     while, if / else if / else chains, &&, ||, !, continue, early returns
 *   B  gcd              binary GCD: do-while with nested whiles, if without else
 *   C  isqrt            binary search: while with if / else
 *   D  utf8_decode      switch with fallthrough (ConvertUTF style), break out of a for loop
 *   E  duff_copy        Duff's device: switch cases interleaved with a do-while
 *   F  rpn_eval         sparse switch, grouped case labels, default, for (;;), goto error path
 *   G  vm_run           bytecode interpreter with computed goto (labels as values)
 *   H  sort / search    nested while with short-circuit, signed vs. unsigned comparisons
 *   I  two_sum          nested for loops with continue and goto out of both loops
 *   J  calendar         leap year &&/|| logic, ternary, dense switch with fallthrough and default
 *   K  bits             while (x), unsigned count-down loop (i-- > 0) with wraparound
 *   L  ackermann        deep recursion, else if
 *   M  http_msg         sparse switch with a default that falls back to range checks
 *   N  asm branches     every branch (and pseudo branch) through `asm goto` on boundary values
 *   O  char_class       asm branch ladder exiting to several C labels
 *   P  find_byte        asm goto early exit out of a C loop
 *   Q  B-type edges     beq/bne/blt/bge/bltu/bgeu at exactly +4094 and -4096
 *   R  C-type edges     c.beqz/c.bnez at +254/-256, c.j at +2046/-2048, c.jal (rv32) with link check
 *   S  J-type edges     jal at exactly +1048574 and -1048576 with link check
 *   T  jalr edges       imm +2047/-2048, bit 0 clearing, rd == rs1, c.jalr ra, c.jr
 *   U  asm -> C edges   asm branches to C labels at +4094, -4096 and (compressed) -256
 *   V  C -> asm labels  C `goto *` into a label defined in assembly (other function and mid-asm)
 *
 * The edge distance tests are padded with zero bytes, which decode as illegal instructions, so taking
 * a wrong target does not silently continue. The offsets of all edge branches are checked after
 * building with an objdump scan, see the build commands below.
 *
 * Build (from test/bins):
 * riscv64-linux-gnu-gcc -O1 -nostdlib -static -fno-pie -no-pie -ffreestanding -fno-tree-loop-distribute-patterns -march=rv32imc -mabi=ilp32 -Wl,--no-relax -o elf/riscv/emulateme_cf.rv32imc src/rzil/riscv/emulateme_cf.c
 * riscv64-linux-gnu-gcc -O1 -nostdlib -static -fno-pie -no-pie -ffreestanding -fno-tree-loop-distribute-patterns -march=rv64imc -mabi=lp64 -Wl,--no-relax -o elf/riscv/emulateme_cf.rv64imc src/rzil/riscv/emulateme_cf.c
 *
 * Linker relaxation is disabled so that no code depends on `gp`, the emulation only needs `sp`.
 * Run natively with: qemu-riscv32 elf/riscv/emulateme_cf.rv32imc; echo $?
 */

typedef unsigned char u8;
typedef int i32;
typedef unsigned int u32;

#define NOINLINE __attribute__((noinline))
#define INT32_MIN_ (-0x7fffffff - 1)
#define INT32_MAX_ 0x7fffffff
#define LONG_MIN_ (-LONG_MAX_ - 1)
#define LONG_MAX_ ((long)(~0UL >> 1))

#if __riscv_xlen == 64
#define LREG "ld"
#define SREG "sd"
#else
#define LREG "lw"
#define SREG "sw"
#endif

/* One slot per check, see the table above. */
char report[] = "......................";

/* ---------------------------------------------------------------------------------------------- */
/* A: number parsing */

/* Parses an optionally signed decimal, 0x hex or 0b binary literal with '_' digit separators. */
NOINLINE int parse_number(const char *s, long *out) {
	int neg = 0;
	unsigned long base = 10, v = 0;
	while (*s == ' ' || *s == '\t') {
		s++;
	}
	if (*s == '-') {
		neg = 1;
		s++;
	} else if (*s == '+') {
		s++;
	}
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
		base = 16;
		s += 2;
	} else if (s[0] == '0' && s[1] == 'b') {
		base = 2;
		s += 2;
	}
	if (!*s) {
		return 0;
	}
	while (*s) {
		char c = *s++;
		unsigned long d;
		if (c == '_') {
			continue;
		}
		if (c >= '0' && c <= '9') {
			d = c - '0';
		} else if (c >= 'a' && c <= 'f') {
			d = c - 'a' + 10;
		} else if (c >= 'A' && c <= 'F') {
			d = c - 'A' + 10;
		} else {
			return 0;
		}
		if (d >= base) {
			return 0;
		}
		v = v * base + d;
	}
	*out = neg ? -(long)v : (long)v;
	return 1;
}

static int test_parse(void) {
	long v = 0;
	int ok = 1;
	ok &= parse_number("  -0x7f_ff", &v) && v == -32767;
	ok &= parse_number("0b1011", &v) && v == 11;
	ok &= parse_number("+123", &v) && v == 123;
	ok &= parse_number("9_9", &v) && v == 99;
	ok &= !parse_number("12z", &v);
	ok &= !parse_number("-", &v);
	ok &= !parse_number("0x", &v);
	ok &= !parse_number("0b102", &v);
	return ok;
}

/* ---------------------------------------------------------------------------------------------- */
/* B: binary GCD */

NOINLINE unsigned long gcd(unsigned long a, unsigned long b) {
	unsigned shift = 0;
	if (a == 0) {
		return b;
	}
	if (b == 0) {
		return a;
	}
	while (((a | b) & 1) == 0) {
		a >>= 1;
		b >>= 1;
		shift++;
	}
	while ((a & 1) == 0) {
		a >>= 1;
	}
	do {
		while ((b & 1) == 0) {
			b >>= 1;
		}
		if (a > b) {
			unsigned long t = a;
			a = b;
			b = t;
		}
		b -= a;
	} while (b != 0);
	return a << shift;
}

static int test_gcd(void) {
	return gcd(48, 180) == 12 && gcd(0, 7) == 7 && gcd(7, 0) == 7 && gcd(1UL << 20, 3UL << 18) == 1UL << 18 &&
		gcd(17, 5) == 1 && gcd(270, 192) == 6;
}

/* ---------------------------------------------------------------------------------------------- */
/* C: integer square root */

NOINLINE u32 isqrt(u32 x) {
	u32 lo = 0, hi = 0x10000;
	while (lo + 1 < hi) {
		u32 mid = (lo + hi) / 2;
		if (mid <= x / mid) {
			lo = mid;
		} else {
			hi = mid;
		}
	}
	return lo;
}

static int test_isqrt(void) {
	return isqrt(0) == 0 && isqrt(1) == 1 && isqrt(15) == 3 && isqrt(16) == 4 && isqrt(17) == 4 &&
		isqrt(0xfffe0000u) == 65534 && isqrt(0xfffe0001u) == 65535 && isqrt(0xffffffffu) == 65535;
}

/* ---------------------------------------------------------------------------------------------- */
/* D: UTF-8 decoding */

static const u32 utf8_offsets[5] = { 0, 0x00000000, 0x00003080, 0x000e2080, 0x03c82080 };

/* Decodes up to max code points, malformed sequences become U+FFFD. */
NOINLINE int utf8_decode(const u8 *s, u32 *out, int max) {
	int n = 0;
	while (*s && n < max) {
		u32 c = *s, cp = 0;
		int len, i;
		if (c < 0x80) {
			len = 1;
		} else if ((c & 0xe0) == 0xc0) {
			len = 2;
		} else if ((c & 0xf0) == 0xe0) {
			len = 3;
		} else if ((c & 0xf8) == 0xf0) {
			len = 4;
		} else {
			len = 0;
		}
		for (i = 1; i < len; i++) {
			if ((s[i] & 0xc0) != 0x80) {
				len = 0;
				break;
			}
		}
		switch (len) {
		case 4:
			cp += *s++;
			cp <<= 6;
			/* fallthrough */
		case 3:
			cp += *s++;
			cp <<= 6;
			/* fallthrough */
		case 2:
			cp += *s++;
			cp <<= 6;
			/* fallthrough */
		case 1:
			cp += *s++;
			cp -= utf8_offsets[len];
			break;
		default:
			cp = 0xfffd;
			s++;
			break;
		}
		out[n++] = cp;
	}
	return n;
}

static int test_utf8(void) {
	static const u8 text[] = "A\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80\xff\xc3" "A";
	static const u32 expect[] = { 0x41, 0xe9, 0x20ac, 0x1f600, 0xfffd, 0xfffd, 0x41 };
	u32 cps[8];
	int n = utf8_decode(text, cps, 8);
	if (n != 7) {
		return 0;
	}
	for (int i = 0; i < n; i++) {
		if (cps[i] != expect[i]) {
			return 0;
		}
	}
	return 1;
}

/* ---------------------------------------------------------------------------------------------- */
/* E: Duff's device */

NOINLINE void duff_copy(u8 *to, const u8 *from, int count) {
	int n;
	if (count <= 0) {
		return;
	}
	n = (count + 7) / 8;
	switch (count % 8) {
	case 0: do { *to++ = *from++;
	/* fallthrough */
	case 7: *to++ = *from++;
	/* fallthrough */
	case 6: *to++ = *from++;
	/* fallthrough */
	case 5: *to++ = *from++;
	/* fallthrough */
	case 4: *to++ = *from++;
	/* fallthrough */
	case 3: *to++ = *from++;
	/* fallthrough */
	case 2: *to++ = *from++;
	/* fallthrough */
	case 1: *to++ = *from++;
		} while (--n > 0);
	}
}

static int test_duff(void) {
	static const u8 src[] = "The quick brown fox";
	static const int counts[] = { 0, 1, 8, 13, 19 };
	for (int c = 0; c < 5; c++) {
		u8 dst[24];
		for (int i = 0; i < 24; i++) {
			dst[i] = '#';
		}
		duff_copy(dst, src, counts[c]);
		for (int i = 0; i < 24; i++) {
			if (dst[i] != (i < counts[c] ? src[i] : '#')) {
				return 0;
			}
		}
	}
	return 1;
}

/* ---------------------------------------------------------------------------------------------- */
/* F: reverse polish notation calculator */

enum {
	RPN_OK,
	RPN_UNDERFLOW,
	RPN_OVERFLOW,
	RPN_DIV0,
	RPN_BADTOK,
};

/* Evaluates single-space separated RPN, '~' negates the top of the stack. */
NOINLINE int rpn_eval(const char *p, long *result) {
	long st[8];
	int sp = 0, err;
	for (;;) {
		char c = *p++;
		long a, b;
		if (c == '\0') {
			break;
		}
		if (c == ' ') {
			continue;
		}
		if (c >= '0' && c <= '9') {
			long v = c - '0';
			while (*p >= '0' && *p <= '9') {
				v = v * 10 + (*p++ - '0');
			}
			if (sp == 8) {
				err = RPN_OVERFLOW;
				goto fail;
			}
			st[sp++] = v;
			continue;
		}
		if (c == '~') {
			if (sp < 1) {
				err = RPN_UNDERFLOW;
				goto fail;
			}
			st[sp - 1] = -st[sp - 1];
			continue;
		}
		if (sp < 2) {
			err = RPN_UNDERFLOW;
			goto fail;
		}
		b = st[--sp];
		a = st[--sp];
		switch (c) {
		case '+':
			a += b;
			break;
		case '-':
			a -= b;
			break;
		case '*':
			a *= b;
			break;
		case '/':
		case '%':
			if (b == 0) {
				err = RPN_DIV0;
				goto fail;
			}
			a = c == '/' ? a / b : a % b;
			break;
		case '&':
			a &= b;
			break;
		case '|':
			a |= b;
			break;
		case '^':
			a ^= b;
			break;
		case '<':
			a = a < b;
			break;
		default:
			err = RPN_BADTOK;
			goto fail;
		}
		st[sp++] = a;
	}
	if (sp != 1) {
		err = RPN_UNDERFLOW;
		goto fail;
	}
	*result = st[0];
	return RPN_OK;
fail:
	*result = 0;
	return err;
}

static int test_rpn(void) {
	long r = 0;
	int ok = 1;
	ok &= rpn_eval("3 4 + 2 *", &r) == RPN_OK && r == 14;
	ok &= rpn_eval("7~ 2 /", &r) == RPN_OK && r == -3;
	ok &= rpn_eval("7~ 2 %", &r) == RPN_OK && r == -1;
	ok &= rpn_eval("5~ 3 <", &r) == RPN_OK && r == 1;
	ok &= rpn_eval("6 3 ^ 12 |", &r) == RPN_OK && r == 13;
	ok &= rpn_eval("100 7 % 3 &", &r) == RPN_OK && r == 2;
	ok &= rpn_eval("1 0 /", &r) == RPN_DIV0;
	ok &= rpn_eval("1 +", &r) == RPN_UNDERFLOW;
	ok &= rpn_eval("1 2", &r) == RPN_UNDERFLOW;
	ok &= rpn_eval("2 3 $", &r) == RPN_BADTOK;
	ok &= rpn_eval("1 2 3 4 5 6 7 8 9", &r) == RPN_OVERFLOW;
	return ok;
}

/* ---------------------------------------------------------------------------------------------- */
/* G: computed goto bytecode interpreter */

enum {
	OP_LI,
	OP_MOV,
	OP_ADD,
	OP_DEC,
	OP_JNZ,
	OP_HALT,
};

typedef struct {
	u8 op, a, b;
	i32 imm;
} Insn;

NOINLINE long vm_run(const Insn *prog) {
	static void *const dispatch[] = { &&op_li, &&op_mov, &&op_add, &&op_dec, &&op_jnz, &&op_halt };
	long r[4] = { 0 };
	const Insn *pc = prog;
#define NEXT goto *dispatch[pc->op]
	NEXT;
op_li:
	r[pc->a] = pc->imm;
	pc++;
	NEXT;
op_mov:
	r[pc->a] = r[pc->b];
	pc++;
	NEXT;
op_add:
	r[pc->a] += r[pc->b];
	pc++;
	NEXT;
op_dec:
	r[pc->a]--;
	pc++;
	NEXT;
op_jnz:
	pc = r[pc->a] ? prog + pc->imm : pc + 1;
	NEXT;
op_halt:
	return r[pc->a];
#undef NEXT
}

static long vm_fib(i32 n) {
	/* n >= 1: a = 0, b = 1; repeat n times { t = a + b; a = b; b = t; } return a */
	const Insn prog[] = {
		{ OP_LI, 0, 0, 0 },
		{ OP_LI, 1, 0, 1 },
		{ OP_LI, 2, 0, n },
		{ OP_MOV, 3, 1, 0 },
		{ OP_ADD, 3, 0, 0 },
		{ OP_MOV, 0, 1, 0 },
		{ OP_MOV, 1, 3, 0 },
		{ OP_DEC, 2, 0, 0 },
		{ OP_JNZ, 2, 0, 3 },
		{ OP_HALT, 0, 0, 0 },
	};
	return vm_run(prog);
}

static int test_vm(void) {
	return vm_fib(1) == 1 && vm_fib(10) == 55 && vm_fib(30) == 832040;
}

/* ---------------------------------------------------------------------------------------------- */
/* H: sorting and searching */

NOINLINE void insertion_sort(i32 *v, int n) {
	int i = 1;
	while (i < n) {
		i32 x = v[i];
		int j = i - 1;
		while (j >= 0 && v[j] > x) {
			v[j + 1] = v[j];
			j--;
		}
		v[j + 1] = x;
		i++;
	}
}

NOINLINE int lower_bound(const i32 *v, int n, i32 key) {
	int lo = 0, hi = n;
	while (lo < hi) {
		int mid = lo + (hi - lo) / 2;
		if (v[mid] < key) {
			lo = mid + 1;
		} else {
			hi = mid;
		}
	}
	return lo;
}

/* Counts elements below key when both are reinterpreted as unsigned. */
NOINLINE int count_below_unsigned(const i32 *v, int n, i32 key) {
	int c = 0;
	for (int i = 0; i < n; i++) {
		if ((u32)v[i] < (u32)key) {
			c++;
		}
	}
	return c;
}

static int test_sort(void) {
	i32 v[] = { 5, -3, INT32_MAX_, INT32_MIN_, 0, -1, 42, 7 };
	static const i32 sorted[] = { INT32_MIN_, -3, -1, 0, 5, 7, 42, INT32_MAX_ };
	insertion_sort(v, 8);
	for (int i = 0; i < 8; i++) {
		if (v[i] != sorted[i]) {
			return 0;
		}
	}
	return lower_bound(v, 8, -2) == 2 && lower_bound(v, 8, 0) == 3 && lower_bound(v, 8, INT32_MIN_) == 0 &&
		lower_bound(v, 8, INT32_MAX_) == 7 && lower_bound(v, 8, 43) == 7 &&
		count_below_unsigned(v, 8, 5) == 1 && count_below_unsigned(v, 8, -1) == 7 &&
		count_below_unsigned(v, 8, INT32_MIN_) == 5;
}

/* ---------------------------------------------------------------------------------------------- */
/* I: pair search */

/* Finds the first pair i < j with v[i] + v[j] == target, negative entries are deleted ones. */
NOINLINE int two_sum(const int *v, int n, int target) {
	int i, j;
	for (i = 0; i < n; i++) {
		if (v[i] < 0) {
			continue;
		}
		for (j = i + 1; j < n; j++) {
			if (v[j] < 0) {
				continue;
			}
			if (v[i] + v[j] == target) {
				goto found;
			}
		}
	}
	return -1;
found:
	return i * 16 + j;
}

static int test_two_sum(void) {
	static const int v[] = { 3, -1, 8, 11, -1, 4, 6 };
	return two_sum(v, 7, 10) == 5 * 16 + 6 && two_sum(v, 7, 14) == 0 * 16 + 3 && two_sum(v, 7, 100) == -1 &&
		two_sum(v, 7, 2) == -1;
}

/* ---------------------------------------------------------------------------------------------- */
/* J: calendar */

NOINLINE int is_leap(int y) {
	return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

/* Sakamoto's algorithm, 0 = Sunday. */
NOINLINE int day_of_week(int y, int m, int d) {
	static const u8 t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	y -= m < 3;
	return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

/* Number of working days (Mon - Fri) left in the week, starting with day d itself. */
NOINLINE int workdays_left(int d) {
	int n = 0;
	switch (d) {
	case 1:
		n++;
		/* fallthrough */
	case 2:
		n++;
		/* fallthrough */
	case 3:
		n++;
		/* fallthrough */
	case 4:
		n++;
		/* fallthrough */
	case 5:
		n++;
		break;
	case 0:
	case 6:
		n = 0;
		break;
	default:
		n = -1;
		break;
	}
	return n;
}

static int test_calendar(void) {
	int leaps = 0, sum = 0;
	for (int y = 1890; y <= 2110; y++) {
		leaps += is_leap(y);
	}
	for (int d = 0; d <= 7; d++) {
		sum = sum * 8 + workdays_left(d) + 1;
	}
	return leaps == 53 && day_of_week(1970, 1, 1) == 4 && day_of_week(2000, 2, 29) == 2 &&
		day_of_week(2026, 10, 18) == 0 && day_of_week(2024, 3, 1) == 5 && sum == 016543210;
}

/* ---------------------------------------------------------------------------------------------- */
/* K: bit fiddling */

NOINLINE int popcount(unsigned long x) {
	int n = 0;
	while (x) {
		x &= x - 1;
		n++;
	}
	return n;
}

NOINLINE void reverse(u8 *p, u32 n) {
	u32 i = 0;
	for (u32 j = n; j-- > i; i++) {
		u8 t = p[i];
		p[i] = p[j];
		p[j] = t;
	}
}

static int test_bits(void) {
	u8 s[] = "RzIL!";
	u8 e[] = "";
	u8 one[] = "x";
	reverse(s, 5);
	reverse(e, 0);
	reverse(one, 1);
	return popcount(0) == 0 && popcount(0xf0f0f0f0UL) == 16 && popcount(~0UL) == __riscv_xlen &&
		s[0] == '!' && s[1] == 'L' && s[2] == 'I' && s[3] == 'z' && s[4] == 'R' && s[5] == 0 && e[0] == 0 &&
		one[0] == 'x';
}

/* ---------------------------------------------------------------------------------------------- */
/* L: recursion */

NOINLINE unsigned long ackermann(unsigned long m, unsigned long n) {
	if (m == 0) {
		return n + 1;
	} else if (n == 0) {
		return ackermann(m - 1, 1);
	} else {
		return ackermann(m - 1, ackermann(m, n - 1));
	}
}

static int test_ackermann(void) {
	return ackermann(0, 0) == 1 && ackermann(1, 5) == 7 && ackermann(2, 3) == 9;
}

/* ---------------------------------------------------------------------------------------------- */
/* M: sparse switch */

/* Maps an HTTP status to a message id, unknown codes map to their class (101 - 105). */
NOINLINE int http_msg(int code) {
	switch (code) {
	case 200: return 1;
	case 201: return 2;
	case 204: return 3;
	case 301: return 4;
	case 302: return 5;
	case 304: return 6;
	case 400: return 7;
	case 401: return 8;
	case 403: return 9;
	case 404: return 10;
	case 418: return 11;
	case 500: return 12;
	case 503: return 13;
	default:
		if (code >= 100 && code < 600) {
			return 100 + code / 100;
		}
		return -1;
	}
}

static int test_http(void) {
	static const int codes[] = { 200, 404, 418, 503, 302, 299, 600, 99, 150, 204, 501 };
	static const int ids[] = { 1, 10, 11, 13, 5, 102, -1, -1, 101, 3, 105 };
	for (int i = 0; i < 11; i++) {
		if (http_msg(codes[i]) != ids[i]) {
			return 0;
		}
	}
	return 1;
}

/* ---------------------------------------------------------------------------------------------- */
/* N: every conditional branch through asm goto, on boundary values */

#define BR2(name, insn) \
	static NOINLINE int name(long a, long b) { \
		__asm__ goto(insn " %0, %1, %l[taken]" ::"r"(a), "r"(b)::taken); \
		return 0; \
	taken: \
		return 1; \
	}
#define BR1(name, insn) \
	static NOINLINE int name(long a) { \
		__asm__ goto(insn " %0, %l[taken]" ::"r"(a)::taken); \
		return 0; \
	taken: \
		return 1; \
	}

BR2(br_beq, "beq")
BR2(br_bne, "bne")
BR2(br_blt, "blt")
BR2(br_bge, "bge")
BR2(br_bltu, "bltu")
BR2(br_bgeu, "bgeu")
BR2(br_bgt, "bgt")
BR2(br_ble, "ble")
BR2(br_bgtu, "bgtu")
BR2(br_bleu, "bleu")
BR1(br_beqz, "beqz")
BR1(br_bnez, "bnez")
BR1(br_bltz, "bltz")
BR1(br_bgez, "bgez")
BR1(br_blez, "blez")
BR1(br_bgtz, "bgtz")

static const long cmp_vals[] = {
	0,
	1,
	-1,
	LONG_MIN_,
	LONG_MAX_,
	(long)0x7fffffffUL,
	(long)0x80000000UL,
#if __riscv_xlen == 64
	(long)0x100000000UL, /* differs from 0 only in the upper word */
	(long)0xffffffff00000000UL,
#endif
};

static int test_asm_branches(void) {
	const int n = sizeof(cmp_vals) / sizeof(cmp_vals[0]);
	for (int i = 0; i < n; i++) {
		long a = cmp_vals[i];
		if (br_beqz(a) != (a == 0) || br_bnez(a) != (a != 0) || br_bltz(a) != (a < 0) ||
			br_bgez(a) != (a >= 0) || br_blez(a) != (a <= 0) || br_bgtz(a) != (a > 0)) {
			return 0;
		}
		for (int j = 0; j < n; j++) {
			long b = cmp_vals[j];
			unsigned long ua = a, ub = b;
			if (br_beq(a, b) != (a == b) || br_bne(a, b) != (a != b) || br_blt(a, b) != (a < b) ||
				br_bge(a, b) != (a >= b) || br_bltu(a, b) != (ua < ub) || br_bgeu(a, b) != (ua >= ub) ||
				br_bgt(a, b) != (a > b) || br_ble(a, b) != (a <= b) || br_bgtu(a, b) != (ua > ub) ||
				br_bleu(a, b) != (ua <= ub)) {
				return 0;
			}
		}
	}
	return 1;
}

/* ---------------------------------------------------------------------------------------------- */
/* O: asm branch ladder with several C label exits */

enum {
	CLASS_DIGIT,
	CLASS_UPPER,
	CLASS_LOWER,
	CLASS_SPACE,
	CLASS_OTHER,
};

/* Range checks use the unsigned trick: (c - lo) <u (hi - lo + 1). */
NOINLINE int char_class(long c) {
	__asm__ goto(
		"addi t0, %0, -48\n"
		"li t1, 10\n"
		"bltu t0, t1, %l[digit]\n"
		"addi t0, %0, -65\n"
		"li t1, 26\n"
		"bltu t0, t1, %l[upper]\n"
		"addi t0, %0, -97\n"
		"bltu t0, t1, %l[lower]\n"
		"li t1, 32\n"
		"beq %0, t1, %l[space]\n"
		:
		: "r"(c)
		: "t0", "t1"
		: digit, upper, lower, space);
	return CLASS_OTHER;
digit:
	return CLASS_DIGIT;
upper:
	return CLASS_UPPER;
lower:
	return CLASS_LOWER;
space:
	return CLASS_SPACE;
}

static int test_char_class(void) {
	static const char text[] = "Hello, RzIL 2026!\tok";
	int counts[5] = { 0 };
	for (const char *p = text; *p; p++) {
		counts[char_class(*p)]++;
	}
	return counts[CLASS_DIGIT] == 4 && counts[CLASS_UPPER] == 4 && counts[CLASS_LOWER] == 7 &&
		counts[CLASS_SPACE] == 2 && counts[CLASS_OTHER] == 3;
}

/* ---------------------------------------------------------------------------------------------- */
/* P: asm goto out of a C loop */

NOINLINE long find_byte(const u8 *p, long n, long c) {
	long i;
	for (i = 0; i < n; i++) {
		__asm__ goto("beq %0, %1, %l[found]" ::"r"((long)p[i]), "r"(c)::found);
	}
	return -1;
found:
	return i;
}

static int test_find_byte(void) {
	static const u8 s[] = "emulate me, RISC-V";
	return find_byte(s, 18, 'R') == 12 && find_byte(s, 18, 'e') == 0 && find_byte(s, 18, 'V') == 17 &&
		find_byte(s, 18, 'x') == -1 && find_byte(s, 0, 'e') == -1;
}

/* ---------------------------------------------------------------------------------------------- */
/* Q: B-type branches at the limits of their 13 bit signed offset */

/* Called with a == b == -1 and c == 0. */
NOINLINE unsigned long asm_btype_edges(long a, long b, long c) {
	unsigned long m;
	__asm__ volatile(
		".option push\n"
		".option norelax\n"
		".option norvc\n"
		"	li %[m], 0\n"
		/* beq, +4094, taken */
		"10:	beq %[a], %[b], 11f\n"
		"	jal zero, 12f\n"
		"	.skip 4094 - (. - 10b), 0\n"
		"11:	ori %[m], %[m], 0x1\n"
		"12:\n"
		/* bne, -4096, taken */
		"	jal zero, 22f\n"
		"21:	ori %[m], %[m], 0x2\n"
		"	jal zero, 23f\n"
		"	.skip 4096 - (. - 21b), 0\n"
		"22:	bne %[a], %[c], 21b\n"
		"23:\n"
		/* bltu, +4094, equal operands: falls through */
		"30:	bltu %[a], %[b], 31f\n"
		"	ori %[m], %[m], 0x4\n"
		"	jal zero, 32f\n"
		"	.skip 4094 - (. - 30b), 0\n"
		"31:	ori %[m], %[m], 0x8\n"
		"32:\n"
		/* bge, -4096, 0 >= -1 only holds signed: taken */
		"	jal zero, 42f\n"
		"41:	ori %[m], %[m], 0x10\n"
		"	jal zero, 43f\n"
		"	.skip 4096 - (. - 41b), 0\n"
		"42:	bge %[c], %[a], 41b\n"
		"43:\n"
		/* bgeu, +4094, 0 >= 0xff..ff unsigned is false: falls through */
		"50:	bgeu %[c], %[a], 51f\n"
		"	ori %[m], %[m], 0x20\n"
		"	jal zero, 52f\n"
		"	.skip 4094 - (. - 50b), 0\n"
		"51:	ori %[m], %[m], 0x40\n"
		"52:\n"
		/* blt, -4096, -1 < 0 signed: taken */
		"	jal zero, 62f\n"
		"61:	ori %[m], %[m], 0x80\n"
		"	jal zero, 63f\n"
		"	.skip 4096 - (. - 61b), 0\n"
		"62:	blt %[a], %[c], 61b\n"
		"63:\n"
		".option pop\n"
		: [m] "=&r"(m)
		: [a] "r"(a), [b] "r"(b), [c] "r"(c));
	return m;
}

static int test_btype_edges(void) {
	return asm_btype_edges(-1, -1, 0) == 0xb7;
}

/* ---------------------------------------------------------------------------------------------- */
/* R: compressed branches and jumps at the limits of their offsets */

NOINLINE unsigned long asm_ctype_edges(long zero, long nonzero) {
	/* c.beqz / c.bnez only encode x8 - x15 */
	register long z __asm__("a2") = zero;
	register long nz __asm__("a3") = nonzero;
	unsigned long m;
	__asm__ volatile(
		".option push\n"
		".option norelax\n"
		".option rvc\n"
		"	li %[m], 0\n"
		/* c.beqz, +254, taken */
		"10:	c.beqz %[z], 11f\n"
		"	c.j 12f\n"
		"	.skip 254 - (. - 10b), 0\n"
		"11:	ori %[m], %[m], 0x1\n"
		"12:\n"
		/* c.bnez, -256, taken */
		"	c.j 22f\n"
		"21:	ori %[m], %[m], 0x2\n"
		"	c.j 23f\n"
		"	.skip 256 - (. - 21b), 0\n"
		"22:	c.bnez %[nz], 21b\n"
		"23:\n"
		/* c.beqz, +254, non-zero register: falls through */
		"30:	c.beqz %[nz], 31f\n"
		"	ori %[m], %[m], 0x4\n"
		"	c.j 32f\n"
		"	.skip 254 - (. - 30b), 0\n"
		"31:	ori %[m], %[m], 0x8\n"
		"32:\n"
		/* c.j, +2046 */
		"40:	c.j 41f\n"
		"	.skip 2046 - (. - 40b), 0\n"
		"41:	ori %[m], %[m], 0x10\n"
		/* c.j, -2048, and the c.j leaving the landing pad is at +2046 again */
		"	.option push\n"
		"	.option norvc\n"
		"	jal zero, 52f\n"
		"	.option pop\n"
		"51:	ori %[m], %[m], 0x20\n"
		"	c.j 53f\n"
		"	.skip 2048 - (. - 51b), 0\n"
		"52:	c.j 51b\n"
		"53:\n"
#if __riscv_xlen == 32
		/* c.jal (rv32 only), +2046 and -2048, checking the link register each time */
		"60:	c.jal 61f\n"
		"62:	lla t1, 67f\n"
		"	bne ra, t1, 63f\n"
		"	ori %[m], %[m], 0x80\n"
		"63:	lla t1, 69f\n"
		"	jr t1\n"
		"	.skip 2046 - (. - 60b), 0\n"
		"61:	c.j 64f\n"
		"	.skip 2, 0\n"
		"66:	c.jal 62b\n"
		"67:\n"
		"64:	lla t1, 62b\n"
		"	bne ra, t1, 65f\n"
		"	ori %[m], %[m], 0x40\n"
		"65:	c.j 66b\n"
		"69:\n"
#endif
		".option pop\n"
		: [m] "=&r"(m)
		: [z] "r"(z), [nz] "r"(nz)
		: "t1", "ra");
	return m;
}

static int test_ctype_edges(void) {
#if __riscv_xlen == 32
	return asm_ctype_edges(0, 0x1234) == 0xf7;
#else
	return asm_ctype_edges(0, 0x1234) == 0x37;
#endif
}

/* ---------------------------------------------------------------------------------------------- */
/* S: jal at the limits of its 21 bit signed offset */

/*
 * This function is over 1 MiB large, so it lives in its own section that is linked after .text,
 * otherwise it would push the other jal instructions (e.g. in rv_island) out of their range.
 */
__attribute__((section(".text.far"))) NOINLINE unsigned long asm_jtype_edges(void) {
	unsigned long m;
	__asm__ volatile(
		".option push\n"
		".option norelax\n"
		".option norvc\n"
		"	li %[m], 0\n"
		/* jal t0, +1048574 */
		"10:	jal t0, 11f\n"
		/* landing pad of the backward jal t2, which must link to 14 */
		"12:	lla t1, 14f\n"
		"	bne t2, t1, 16f\n"
		"	ori %[m], %[m], 0x2\n"
		"16:	lla t1, 19f\n"
		"	jr t1\n"
		"	.skip 1048574 - (. - 10b), 0\n"
		"11:	jal zero, 14f\n"
		"	.skip 2, 0\n"
		/* jal t2, -1048576 */
		"13:	jal t2, 12b\n"
		"14:	lla t1, 12b\n"
		"	bne t0, t1, 15f\n"
		"	ori %[m], %[m], 0x1\n"
		"15:	jal zero, 13b\n"
		"19:\n"
		".option pop\n"
		: [m] "=&r"(m)
		:
		: "t0", "t1", "t2");
	return m;
}

static int test_jtype_edges(void) {
	return asm_jtype_edges() == 0x3;
}

/* ---------------------------------------------------------------------------------------------- */
/* T: indirect jumps */

NOINLINE unsigned long asm_jalr_edges(void) {
	unsigned long m;
	__asm__ volatile(
		".option push\n"
		".option norelax\n"
		".option rvc\n"
		"	li %[m], 0\n"
		/* largest immediate: (t1 + 2047) & ~1 lands on 11 */
		"	lla t1, 11f\n"
		"	addi t1, t1, -2046\n"
		"10:	jalr t0, 2047(t1)\n"
		"13:	c.j 12f\n"
		"11:	lla t2, 13b\n"
		"	bne t0, t2, 12f\n"
		"	ori %[m], %[m], 0x1\n"
		"12:\n"
		/* most negative immediate */
		"	lla t1, 21f\n"
		"	addi t1, t1, 2047\n"
		"	addi t1, t1, 1\n"
		"	jalr zero, -2048(t1)\n"
		"	c.j 22f\n"
		"21:	ori %[m], %[m], 0x2\n"
		"22:\n"
		/* rd == rs1: the target comes from the old value */
		"	lla t1, 31f\n"
		"	jalr t1, 0(t1)\n"
		"33:	c.j 32f\n"
		"31:	lla t2, 33b\n"
		"	bne t1, t2, 32f\n"
		"	ori %[m], %[m], 0x4\n"
		"32:\n"
		/* c.jalr ra: the source is also the implicit link register */
		"	lla ra, 41f\n"
		"	c.jalr ra\n"
		"43:	c.j 42f\n"
		"41:	lla t2, 43b\n"
		"	bne ra, t2, 42f\n"
		"	ori %[m], %[m], 0x8\n"
		"42:\n"
		/* c.jr */
		"	lla t1, 51f\n"
		"	c.jr t1\n"
		"	c.j 52f\n"
		"51:	ori %[m], %[m], 0x10\n"
		"52:\n"
		/* odd register value: bit 0 of the target is discarded */
		"	lla t1, 61f\n"
		"	addi t1, t1, 1\n"
		"	jalr zero, 0(t1)\n"
		"	c.j 62f\n"
		"61:	ori %[m], %[m], 0x20\n"
		"62:\n"
		".option pop\n"
		: [m] "=&r"(m)
		:
		: "t0", "t1", "t2", "ra");
	return m;
}

static int test_jalr_edges(void) {
	return asm_jalr_edges() == 0x3f;
}

/* ---------------------------------------------------------------------------------------------- */
/* U: asm branching to C labels at edge distances */

volatile long rv_edge_counter;
volatile long rv_edge_flags;

/*
 * The C label targets have to start exactly where the asm expects them, so this function keeps its
 * results in a volatile global instead of a local (whose value would have to be merged at the labels)
 * and is not subject to basic block reordering. If the label still ends up out of range, gas silently
 * turns the beq into a bne over a jal, which the post build objdump scan catches.
 */
__attribute__((optimize("no-reorder-blocks"))) NOINLINE long asm_to_c_label_edges(long a, long b) {
	rv_edge_flags = 0;
	/* beq to a C label exactly +4094 ahead, the label follows the asm statement directly */
	__asm__ goto(
		".option push\n"
		".option norelax\n"
		".option norvc\n"
		"0:	beq %0, %1, %l[far]\n"
		"	jal zero, %l[near]\n"
		"	.skip 4094 - (. - 0b), 0\n"
		".option pop\n"
		:
		: "r"(a), "r"(b)
		:
		: far, near);
far:
	rv_edge_flags |= 1;
near:
	/* a loop with a C label as its head and a bne in asm as the back-edge, 4096 bytes back */
	rv_edge_counter = 3;
head:
	__asm__ goto(
		".option push\n"
		".option norelax\n"
		".option norvc\n"
		"0:	lla t0, rv_edge_counter\n"
		"	" LREG " t1, 0(t0)\n"
		"	addi t1, t1, -1\n"
		"	" SREG " t1, 0(t0)\n"
		"	jal zero, 1f\n"
		"	.skip 4096 - (. - 0b), 0\n"
		"1:	bnez t1, %l[head]\n"
		".option pop\n"
		:
		:
		: "t0", "t1", "memory"
		: head);
	if (rv_edge_counter == 0) {
		rv_edge_flags |= 2;
	}
	/* the same with a compressed back-edge at its -256 limit */
	rv_edge_counter = 4;
head_c:
	__asm__ goto(
		".option push\n"
		".option norelax\n"
		".option rvc\n"
		"0:	lla a5, rv_edge_counter\n"
		"	" LREG " a4, 0(a5)\n"
		"	addi a4, a4, -1\n"
		"	" SREG " a4, 0(a5)\n"
		"	c.j 1f\n"
		"	.skip 256 - (. - 0b), 0\n"
		"1:	c.bnez a4, %l[head_c]\n"
		".option pop\n"
		:
		:
		: "a4", "a5", "memory"
		: head_c);
	if (rv_edge_counter == 0) {
		rv_edge_flags |= 4;
	}
	return rv_edge_flags;
}

static int test_asm_to_c_edges(void) {
	return asm_to_c_label_edges(5, 5) == 7 && asm_to_c_label_edges(5, 6) == 6;
}

/* ---------------------------------------------------------------------------------------------- */
/* V: C jumping to labels defined in assembly */

volatile long rv_island_io;
volatile long rv_ladder_val;
extern char rv_island[];
extern char rv_ladder_mid[];

/*
 * rv_island scales rv_island_io by 4 if it is positive and by 2 if it is negative, then jumps back
 * into island_dispatch at the asm labels rv_island_back_pos / rv_island_back_neg. It preserves
 * every register, so the C code continues as if it had taken a computed goto to those labels.
 */
__asm__(
	".text\n"
	".globl rv_island\n"
	"rv_island:\n"
	"	addi sp, sp, -16\n"
	"	" SREG " t0, 0(sp)\n"
	"	" SREG " t1, 8(sp)\n"
	"	lla t0, rv_island_io\n"
	"	" LREG " t1, 0(t0)\n"
	"	bltz t1, 1f\n"
	"	slli t1, t1, 2\n"
	"	" SREG " t1, 0(t0)\n"
	"	" LREG " t0, 0(sp)\n"
	"	" LREG " t1, 8(sp)\n"
	"	addi sp, sp, 16\n"
	"	j rv_island_back_pos\n"
	"1:	slli t1, t1, 1\n"
	"	" SREG " t1, 0(t0)\n"
	"	" LREG " t0, 0(sp)\n"
	"	" LREG " t1, 8(sp)\n"
	"	addi sp, sp, 16\n"
	"	j rv_island_back_neg\n");

/* Returns 4v for positive, -2v for negative and 0 for zero values, the sign test runs in rv_island. */
NOINLINE long island_dispatch(long v) {
	/* taking the addresses makes these labels valid targets of the computed goto below */
	static void *volatile returns[2] __attribute__((unused));
	returns[0] = &&back_pos;
	returns[1] = &&back_neg;
	rv_island_io = v;
	if (v != 0) {
		goto *(void *)rv_island;
	}
	return 0;
back_pos:
	__asm__ volatile("rv_island_back_pos:");
	return rv_island_io;
back_neg:
	__asm__ volatile("rv_island_back_neg:");
	return -rv_island_io;
}

/*
 * Even values get +100 and are then doubled, odd values enter the asm half way and are only doubled,
 * negative values are rejected. With a single address-taken label GCC would turn the computed goto
 * into a direct jump to that label, the second one keeps it a real indirect jump.
 */
NOINLINE long ladder(long v) {
	static void *volatile exits[2];
	exits[0] = &&done;
	exits[1] = &&rejected;
	if (v < 0) {
		goto *exits[1];
	}
	rv_ladder_val = v;
	if (v & 1) {
		goto *(void *)rv_ladder_mid;
	}
	__asm__ goto(
		"	lla t0, rv_ladder_val\n"
		"	" LREG " t1, 0(t0)\n"
		"	addi t1, t1, 100\n"
		"	" SREG " t1, 0(t0)\n"
		".globl rv_ladder_mid\n"
		"rv_ladder_mid:\n"
		"	lla t0, rv_ladder_val\n"
		"	" LREG " t1, 0(t0)\n"
		"	slli t1, t1, 1\n"
		"	" SREG " t1, 0(t0)\n"
		"	j %l[done]\n"
		:
		:
		: "t0", "t1", "memory"
		: done);
done:
	return rv_ladder_val;
rejected:
	return -1;
}

static int test_c_to_asm_labels(void) {
	return island_dispatch(5) == 20 && island_dispatch(-3) == 6 && island_dispatch(0) == 0 && ladder(10) == 220 &&
		ladder(7) == 14 && ladder(-5) == -1;
}

/* ---------------------------------------------------------------------------------------------- */

typedef int (*TestFn)(void);

static const TestFn tests[] = {
	test_parse,
	test_gcd,
	test_isqrt,
	test_utf8,
	test_duff,
	test_rpn,
	test_vm,
	test_sort,
	test_two_sum,
	test_calendar,
	test_bits,
	test_ackermann,
	test_http,
	test_asm_branches,
	test_char_class,
	test_find_byte,
	test_btype_edges,
	test_ctype_edges,
	test_jtype_edges,
	test_jalr_edges,
	test_asm_to_c_edges,
	test_c_to_asm_labels,
};

_Static_assert(sizeof(tests) / sizeof(tests[0]) == sizeof(report) - 1, "one report slot per test");

NOINLINE int run_all(void) {
	int failures = 0;
	for (unsigned i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		int ok = tests[i]();
		report[i] = ok ? 'A' + i : '-';
		failures += !ok;
	}
	return failures;
}

/* Emulation runs from _start up to emulateme_done, the rest only matters when executed natively. */
__asm__(
	".text\n"
	".globl _start\n"
	"_start:\n"
	"	call run_all\n"
	".globl emulateme_done\n"
	"emulateme_done:\n"
	"	mv s0, a0\n"
	"	li a0, 1\n"
	"	lla a1, report\n"
	"	li a2, 22\n"
	"	li a7, 64\n" /* write */
	"	ecall\n"
	"	mv a0, s0\n"
	"	li a7, 93\n" /* exit */
	"	ecall\n");
