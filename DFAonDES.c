#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>

#include"des_core.h"

/** just a function to dump string as hex **/
void hexdump(unsigned char *string, int strlen)
{
	int i;
	for(i = 0; i < strlen; i++)
		printf("%02x ",string[i]);
	printf("\n");
}

/** a function to wipe an array **/
void zeroize(unsigned char *string, int strlen)
{
	int i;
	for(i = 0; i < strlen; i++)
		string[i] = 0;
}

/** generates a random DES key **/
BYTE * get_random_key(int keylen, int seed)
{
	BYTE * random_key = calloc(keylen,1);
	int i;

	srand(seed);
	for(i = 0; i < keylen; i++)
		random_key[i] = (rand() >> 24);
	return random_key;
}

/** generates a random 8-bytes messages **/
BYTE * get_random_message(int seed)
{
	return get_random_key(8, seed);
}

/** this function lists all the possible keys **/
void init_key_candidates(unsigned char ** guess_k)
{
	int i;
	for(i = 0; i < 64; i++)
	{
		guess_k[0][i] = 1;
		guess_k[1][i] = 1;
		guess_k[2][i] = 1;
		guess_k[3][i] = 1;
		guess_k[4][i] = 1;
		guess_k[5][i] = 1;
		guess_k[6][i] = 1;
		guess_k[7][i] = 1;
	}
	guess_k[0][64] = 64;
	guess_k[1][64] = 64;
	guess_k[2][64] = 64;
	guess_k[3][64] = 64;
	guess_k[4][64] = 64;
	guess_k[5][64] = 64;
	guess_k[6][64] = 64;
	guess_k[7][64] = 64;
}

/** a function to rebuild the subkey **/
void find_non_zero(unsigned char ** guess_k)
{
	int i, j;
	unsigned char *tmp = calloc(8, 1);
	unsigned char *subkey = calloc(6, 1);
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 64; j++)
		{
			if(guess_k[i][j] != 0)
			{
				tmp[i] = j;
			}
		}
	}
	subkey[0] = ((tmp[0] << 2)) ^ ((tmp[1] & 0x30) >> 4);
	subkey[1] = ((tmp[1] & 0x0F) << 4) ^ ((tmp[2] & 0x3C) >> 2);
	subkey[2] = ((tmp[2] & 0x03) << 6) ^ (tmp[3]);

	subkey[3] = ((tmp[4] << 2)) ^ ((tmp[5] & 0x30) >> 4);
	subkey[4] = ((tmp[5] & 0x0F) << 4) ^ ((tmp[6] & 0x3C) >> 2);
	subkey[5] = ((tmp[6] & 0x03) << 6) ^ (tmp[7]);

	printf("found subkey: \n");
	hexdump(subkey, 6);
}

/** the attack ! **/
void dfa(BYTE *key)
{
	ROUND_BLOCK exp_key[16] = {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
	Key_Expansion(key, exp_key);

	int seed = 255 + getpid();
	int KeyCount = 384, messages_count = 0, fault_index, pos1, pos2, i;
	unsigned char j, *tmp, *exp_p, *exp_q, delta, inp_p, inp_q;
	unsigned char ** guess_k = malloc(8 * sizeof(unsigned char *));

	for(i = 0; i < 8; i++){
		guess_k[i] = calloc(65, 1);
	}
	init_key_candidates(guess_k);

	while(KeyCount > 8)
	{
		printf("picking a random message...\n");
		BYTE * p = get_random_message(seed);
		messages_count++;
		BYTE * q = calloc(8, 1);
		for(i = 0; i < 8; i++)
			q[i] = p[i];

		printf("message encryption...\n");
		des_encryption(exp_key, p);
		seed++;
		printf("message faulty encryption...\n");
		des_faulty_encryption(exp_key, q, seed);

		/* disturbed byte index */
		fault_index = -1;
		if(p[4] ^ q[4])
			fault_index = 0;
		if(p[5] ^ q[5])
			fault_index = 1;
		if(p[6] ^ q[6])
			fault_index = 2;
		if(p[7] ^ q[7])
			fault_index = 3;

		/* disturbed quartets indexes */
		pos1 = -1;
		pos2 = -1;

		/* delta = injected fault */
		delta = (p[fault_index + 4] ^ q[fault_index + 4]);

		if(delta & 0xF0)
			pos1 = 2*fault_index + 1;
		if(delta & 0x0F)
			pos2 = 2*fault_index + 2;

		/* output difference */
		tmp = calloc(4,1);
		tmp[0] = p[0] ^ q[0];
		tmp[1] = p[1] ^ q[1];
		tmp[2] = p[2] ^ q[2];
		tmp[3] = p[3] ^ q[3];

		/* s-box output difference*/
		InversePermutation(tmp);

		exp_p = calloc(6,1);
		exp_q = calloc(6,1);

		if((pos1 == 1))
		{
			Expansion(p+4, exp_p);
			Expansion(q+4, exp_q);

			for(j = 0; j < 64; j++)
			{
				inp_p = (exp_p[0] & (0xFC))>>2;
				inp_q = (exp_q[0] & (0xFC))>>2;
				inp_p ^= j;
				inp_q ^= j;
				if(/* looks if the k[pos] possible byte verifies S[k^p] ^ S[k^q] = delta_out */
					(guess_k[pos1-1][j] != 0)&&((sbox(pos1, inp_p) ^ sbox(pos1, inp_q)) != ((tmp[0] & 0xF0)>>4))
					)
				{
					guess_k[pos1-1][j] = 0;
					guess_k[pos1-1][64] = (guess_k[pos1-1][64]) - 1;
				}
			}
			zeroize(exp_p, 6);
			zeroize(exp_q, 6);
			KeyCount = guess_k[0][64] + guess_k[1][64] + guess_k[2][64] + guess_k[3][64] + 
						guess_k[4][64] + guess_k[5][64] + guess_k[6][64] + guess_k[7][64];
		}

		if((pos2 == 2))
		{
			Expansion(p+4, exp_p);
			Expansion(q+4, exp_q);

			for(j = 0; j < 64; j++)
			{
				inp_p = (((exp_p[0] & 0x03)<<4) ^ ((exp_p[1] & 0xF0)>>4));
				inp_q = (((exp_q[0] & 0x03)<<4) ^ ((exp_q[1] & 0xF0)>>4));
				inp_p ^= j;
				inp_q ^= j;
				if(/* looks if the k[pos] possible byte verifies S[k^p] ^ S[k^q] = delta_out */
					(guess_k[pos2-1][j] != 0)&&((sbox(pos2, inp_p) ^ sbox(pos2, inp_q)) != ((tmp[0] & 0x0F)))
					)
				{
					guess_k[pos2-1][j] = 0;
					guess_k[pos2-1][64] = (guess_k[pos2-1][64]) - 1;
				}
			}
			zeroize(exp_p, 6);
			zeroize(exp_q, 6);
			KeyCount = guess_k[0][64] + guess_k[1][64] + guess_k[2][64] + guess_k[3][64] + 
						guess_k[4][64] + guess_k[5][64] + guess_k[6][64] + guess_k[7][64];
		}

		if((pos1 == 3))
		{
			Expansion(p+4, exp_p);
			Expansion(q+4, exp_q);

			for(j = 0; j < 64; j++)
			{
				inp_p = (((exp_p[1] & (0x0F))<<2) ^ ((exp_p[2] & (0xC0))>>6));
				inp_q = (((exp_q[1] & (0x0F))<<2) ^ ((exp_q[2] & (0xC0))>>6));
				inp_p ^= j;
				inp_q ^= j;
				if(/* looks if the k[pos] possible byte verifies S[k^p] ^ S[k^q] = delta_out */
					(guess_k[pos1-1][j] != 0)&&((sbox(pos1, inp_p) ^ sbox(pos1, inp_q)) != ((tmp[1] & 0xF0)>>4))
					)
				{
					guess_k[pos1-1][j] = 0;
					guess_k[pos1-1][64] = (guess_k[pos1-1][64]) - 1;
				}
			}
		zeroize(exp_p, 6);
		zeroize(exp_q, 6);
		KeyCount = guess_k[0][64] + guess_k[1][64] + guess_k[2][64] + guess_k[3][64] + 
				    guess_k[4][64] + guess_k[5][64] + guess_k[6][64] + guess_k[7][64];
		}

		if((pos2 == 4))
		{
			Expansion(p+4, exp_p);
			Expansion(q+4, exp_q);

			for(j = 0; j < 64; j++)
			{
				inp_p = (exp_p[2] & (0x3F));
				inp_q = (exp_q[2] & (0x3F));
				inp_p ^= j;
				inp_q ^= j;
				if(/* looks if the k[pos] possible byte verifies S[k^p] ^ S[k^q] = delta_out */
					(guess_k[pos2-1][j] != 0)&&((sbox(pos2, inp_p) ^ sbox(pos2, inp_q)) != ((tmp[1] & 0x0F)))
					)
				{
					guess_k[pos2-1][j] = 0;
					guess_k[pos2-1][64] = (guess_k[pos2-1][64]) - 1;
				}
			}
			zeroize(exp_p, 6);
			zeroize(exp_q, 6);
			KeyCount = guess_k[0][64] + guess_k[1][64] + guess_k[2][64] + guess_k[3][64] + 
					guess_k[4][64] + guess_k[5][64] + guess_k[6][64] + guess_k[7][64];
		}
	
		if((pos1 == 5)){
			Expansion(p+4, exp_p);
			Expansion(q+4, exp_q);

			for(j = 0; j < 64; j++){
				inp_p = (exp_p[3] & (0xFC))>>2;
				inp_q = (exp_q[3] & (0xFC))>>2;
				inp_p ^= j;
				inp_q ^= j;
				if(
					(guess_k[pos1-1][j] != 0)&&((sbox(pos1, inp_p) ^ sbox(pos1, inp_q)) != ((tmp[2] & 0xF0)>>4))
					)
				{
					guess_k[pos1-1][j] = 0;
					guess_k[pos1-1][64] = (guess_k[pos1-1][64]) - 1;
				}
			}
			zeroize(exp_p, 6);
			zeroize(exp_q, 6);
			KeyCount = guess_k[0][64] + guess_k[1][64] + guess_k[2][64] + guess_k[3][64] + 
					guess_k[4][64] + guess_k[5][64] + guess_k[6][64] + guess_k[7][64];
		}

		if((pos2 == 6)){
			Expansion(p+4, exp_p);
			Expansion(q+4, exp_q);

			for(j = 0; j < 64; j++)
			{
				inp_p = (((exp_p[3] & 0x03)<<4) ^ ((exp_p[4] & 0xF0)>>4));
				inp_q = (((exp_q[3] & 0x03)<<4) ^ ((exp_q[4] & 0xF0)>>4));
				inp_p ^= j;
				inp_q ^= j;
				if(/* looks if the k[pos] possible byte verifies S[k^p] ^ S[k^q] = delta_out */
					(guess_k[pos2-1][j] != 0)&&((sbox(pos2, inp_p) ^ sbox(pos2, inp_q)) != ((tmp[2] & 0x0F)))
					)
				{
					guess_k[pos2-1][j] = 0;
					guess_k[pos2-1][64] = (guess_k[pos2-1][64]) - 1;
				}
			}
			zeroize(exp_p, 6);
			zeroize(exp_q, 6);
			KeyCount = guess_k[0][64] + guess_k[1][64] + guess_k[2][64] + guess_k[3][64] + 
					guess_k[4][64] + guess_k[5][64] + guess_k[6][64] + guess_k[7][64];
		}

		if((pos1 == 7)){
			Expansion(p+4, exp_p);
			Expansion(q+4, exp_q);

			for(j = 0; j < 64; j++)
			{
				inp_p = (((exp_p[4] & (0x0F))<<2) ^ ((exp_p[5] & (0xC0))>>6));
				inp_q = (((exp_q[4] & (0x0F))<<2) ^ ((exp_q[5] & (0xC0))>>6));
				inp_p ^= j;
				inp_q ^= j;
				if(
					(guess_k[pos1-1][j] != 0)&&((sbox(pos1, inp_p) ^ sbox(pos1, inp_q)) != ((tmp[3] & 0xF0)>>4))
					)
				{
					guess_k[pos1-1][j] = 0;
					guess_k[pos1-1][64] = (guess_k[pos1-1][64]) - 1;
				}
			}
			zeroize(exp_p, 6);
			zeroize(exp_q, 6);
			KeyCount = guess_k[0][64] + guess_k[1][64] + guess_k[2][64] + guess_k[3][64] + 
						guess_k[4][64] + guess_k[5][64] + guess_k[6][64] + guess_k[7][64];
		}

		if((pos2 == 8))
		{
			Expansion(p+4, exp_p);
			Expansion(q+4, exp_q);

			for(j = 0; j < 64; j++)
			{
				inp_p = (exp_p[5] & (0x3F));
				inp_q = (exp_q[5] & (0x3F));
				inp_p ^= j;
				inp_q ^= j;
				if(/* looks if the k[pos] possible byte verifies S[k^p] ^ S[k^q] = delta_out */
					(guess_k[pos2-1][j] != 0)&&((sbox(pos2, inp_p) ^ sbox(pos2, inp_q)) != ((tmp[3] & 0x0F)))
					)
				{
					guess_k[pos2-1][j] = 0;
					guess_k[pos2-1][64] = (guess_k[pos2-1][64]) - 1;
				}
			}
			zeroize(exp_p, 6);
			zeroize(exp_q, 6);
			KeyCount = guess_k[0][64] + guess_k[1][64] + guess_k[2][64] + guess_k[3][64] + 
						guess_k[4][64] + guess_k[5][64] + guess_k[6][64] + guess_k[7][64];
		}
	}
	printf("last round subkey: \n");
	hexdump(exp_key[15],6);

	find_non_zero(guess_k);
	printf("number of messages used: %d \n", messages_count);
}

int main(int argc, char ** argv)
{
	int seed = getpid();
	BYTE * key = get_random_key(8, seed);
	
	printf("real key: \n");
	hexdump(key,8);

	dfa(key);

	return 0;
}
