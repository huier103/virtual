extern unsigned short threegpplte_interleaver_output;
extern unsigned int threegpplte_interleaver_tmp;

extern __inline void threegpplte_interleaver_reset();

extern __inline unsigned short threegpplte_interleaver(unsigned short f1,
					      unsigned short f2,
					      unsigned short K);


extern __inline short threegpp_interleaver_parameters(unsigned short bytes_per_codeword);
