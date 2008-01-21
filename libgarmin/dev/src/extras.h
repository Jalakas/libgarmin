/*
 * Experimental stuff
 */
#ifdef EXTRAS
int fixup_tre(struct hdr_tre_t *tre, unsigned char *data);
#else
static int fixup_tre(struct hdr_tre_t *tre, unsigned char *data) { return 0;};
#endif
