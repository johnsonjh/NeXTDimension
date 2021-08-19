struct user {
	long u_data[(USIZE *NBPC) / sizeof(long)];
	};

#define u_tsize u_data[1200]
#define u_dsize u_data[1201]
#define u_ssize u_data[1202]

#define u_ar0 u_data[1215]
