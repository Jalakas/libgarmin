int gar_rect_contains(struct gar_rect *r1, double lat, double lon);
int gar_rects_intersect(struct gar_rect *r2, struct gar_rect *r1);
void gar_rect_log(int l, char *pref,struct gar_rect *r);
int gar_rects_overlaps(struct gar_rect *r2, struct gar_rect *r1);
int gar_rects_intersectboth(struct gar_rect *r2, struct gar_rect *r1);
