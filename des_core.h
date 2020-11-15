typedef unsigned char BYTE;
typedef BYTE BLOCK[8];
typedef BYTE HALF_BLOCK[4];
typedef BYTE ROUND_BLOCK[6];

void Key_Expansion(BYTE key[8], ROUND_BLOCK K[16]);
void des_encryption(ROUND_BLOCK exp_key[16], BLOCK plaintext);
void des_faulty_encryption(ROUND_BLOCK exp_key[16], BLOCK plaintext, int seed);

void Expansion(HALF_BLOCK right, ROUND_BLOCK expansed);
void Permutation(HALF_BLOCK right);
void InversePermutation(HALF_BLOCK right);
BYTE _sbox_1(BYTE input);
BYTE _sbox_2(BYTE input);
BYTE _sbox_3(BYTE input);
BYTE _sbox_4(BYTE input);
BYTE _sbox_5(BYTE input);
BYTE _sbox_6(BYTE input);
BYTE _sbox_7(BYTE input);
BYTE _sbox_8(BYTE input);
BYTE sbox(int pos, BYTE input);
void DES_SBOX(ROUND_BLOCK right, HALF_BLOCK output);
