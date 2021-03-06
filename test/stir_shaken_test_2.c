#include <stir_shaken.h>

const char *path = "./test/run";

#define BUF_LEN 1000


stir_shaken_status_t stir_shaken_unit_test_passport_create(void)
{
	stir_shaken_passport_t	*passport = NULL;
	stir_shaken_status_t	status = STIR_SHAKEN_STATUS_FALSE;
	stir_shaken_context_t ss = { 0 };
	const char *error_description = NULL;
	stir_shaken_error_t error_code = STIR_SHAKEN_ERROR_GENERAL;
	const char *x5u = "https://cert.example.org/passport.cer";      // ref
	const char *attest = "A";
	const char *desttn_key = "uri";                                 // ref
	const char *desttn_val = "sip:alice@example.com";               // ref
	int iat = 1471375418;                                           // ref
	const char *origtn_key = "tn";                                  // ref test for orig telephone number
	const char *origtn_val = "12155551212";                         // ref
	const char *origid = "Yeah, later";
	uint8_t ppt_ignore = 1;                                         // ignore, ref test case doesn't include this field

	char *h = NULL, *p = NULL, *s = NULL, *out = NULL;

	char private_key_name[300] = { 0 };
	char public_key_name[300] = { 0 };

	unsigned char	priv_raw[STIR_SHAKEN_PRIV_KEY_RAW_BUF_LEN] = { 0 };
	uint32_t		priv_raw_len = STIR_SHAKEN_PRIV_KEY_RAW_BUF_LEN;	

	EC_KEY *ec_key = NULL;
	EVP_PKEY *private_key = NULL;
	EVP_PKEY *public_key = NULL;

	stir_shaken_passport_params_t params = { .x5u = x5u, .attest = attest, .desttn_key = desttn_key, .desttn_val = desttn_val, .iat = iat, .origtn_key = origtn_key, .origtn_val = origtn_val, .origid = origid, .ppt_ignore = ppt_ignore};

	sprintf(private_key_name, "%s%c%s", path, '/', "u2_private_key.pem");
	sprintf(public_key_name, "%s%c%s", path, '/', "u2_public_key.pem");

	printf("=== Unit testing: STIR/Shaken PASSporT create/encode\n\n");

	// Generate new keys for this test
	status = stir_shaken_generate_keys(&ss, &ec_key, &private_key, &public_key, private_key_name, public_key_name, priv_raw, &priv_raw_len);
	if (stir_shaken_is_error_set(&ss)) {
		error_description = stir_shaken_get_error(&ss, &error_code);
		printf("Error description is: '%s'\n", error_description);
		printf("Error code is: '%d'\n", error_code);
	}

	stir_shaken_assert(status == STIR_SHAKEN_STATUS_OK, "Err, failed to generate keys...");
	stir_shaken_assert(ec_key != NULL, "Err, failed to generate EC key");
	stir_shaken_assert(private_key != NULL, "Err, failed to generate private key");
	stir_shaken_assert(public_key != NULL, "Err, failed to generate public key");
	stir_shaken_assert(stir_shaken_is_error_set(&ss) == 0, "Err, error condition set (should not be set)");
	error_description = stir_shaken_get_error(&ss, &error_code);
	stir_shaken_assert(error_code == STIR_SHAKEN_ERROR_GENERAL, "Err, error should be GENERAL");
	stir_shaken_assert(error_description == NULL, "Err, error description set, should be NULL");

	/* Test */
	passport = stir_shaken_passport_create(&ss, &params, priv_raw, priv_raw_len);
	if (stir_shaken_is_error_set(&ss)) {
		error_description = stir_shaken_get_error(&ss, &error_code);
		printf("Error description is: '%s'\n", error_description);
		printf("Error code is: '%d'\n", error_code);
	}
	stir_shaken_assert(stir_shaken_is_error_set(&ss) == 0, "Err, error condition set (should not be set)");
	error_description = stir_shaken_get_error(&ss, &error_code);
	stir_shaken_assert(error_code == STIR_SHAKEN_ERROR_GENERAL, "Err, error should be GENERAL");
	stir_shaken_assert(error_description == NULL, "Err, error description set, should be NULL");

	stir_shaken_assert(status == STIR_SHAKEN_STATUS_OK, "PASSporT has not been created");
	stir_shaken_assert(passport->jwt != NULL, "JWT has not been created");
	s = stir_shaken_passport_dump_str(&ss, passport, 1);
	printf("1. JWT:\n%s\n", s);
	stir_shaken_free_jwt_str(s); s = NULL;

	// Test signatures
	printf("2. Test signatures\n\n");

	out = jwt_encode_str(passport->jwt);
	stir_shaken_assert(out, "JWT PASSporT Sign: Failed to encode JWT");
	printf("JWT signed:\n%s\n", out);

	h = out;
	p = strchr(h, '.');
	if (p) { *p = '\0'; p += 1; }
	s = strchr(p, '.');
	if (s) { *s = '\0'; s += 1; }

	stir_shaken_assert(h != p && p != s && h != NULL && p != NULL && s != NULL, "PASSporT Malformed!");

	printf("\nHeader encoded:\n%s\n", h);
	printf("\nPayload encoded:\n%s\n", p);
	printf("\nSignature:\n%s\n\n", s);

	free(out); out = NULL;

	/* Need to free JSON object. */
	stir_shaken_passport_destroy(&passport);

	stir_shaken_destroy_keys_ex(&ec_key, &private_key, &public_key);

	return status;
}

int main(void)
{
	stir_shaken_assert(STIR_SHAKEN_STATUS_OK == stir_shaken_do_init(NULL, NULL, NULL, STIR_SHAKEN_LOGLEVEL_HIGH), "Cannot init lib");

	if (stir_shaken_dir_exists(path) != STIR_SHAKEN_STATUS_OK) {

		if (stir_shaken_dir_create_recursive(path) != STIR_SHAKEN_STATUS_OK) {

			printf("ERR: Cannot create test dir\n");
			return -1;
		}
	}

	if (stir_shaken_unit_test_passport_create() != STIR_SHAKEN_STATUS_OK) {

		printf("Fail\n");
		return -2;
	}

	stir_shaken_do_deinit();

	printf("OK\n");

	return 0;
}
