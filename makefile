all:
	gcc -o otp_enc_d otp_enc_d.c otp_enc_d_utilities.c -g --std=c99
	gcc -o otp_enc otp_enc.c otp_enc_utilities.c -g --std=c99
	gcc -o otp_dec_d otp_dec_d.c otp_dec_d_utilities.c -g --std=c99
	gcc -o otp_dec otp_dec.c otp_dec_utilities.c -g --std=c99
	gcc -o keygen keygen.c -g --std=c99

clean:
	rm -f *.o otp_enc_d otp_enc otp_dec_d otp_dec keygen d_cyphertext d_key d_plaintext dec_cyphertext dec_key dec_plaintext