int gar_load_subdiv_data(struct gar_subfile *sub, struct gar_subdiv *gsub);
void gar_free_subdiv_data(struct gar_subdiv *gsd);
//void gar_subdiv_ref(struct gar_subdiv *sd);
//void gar_subdiv_unref(struct gar_subdiv *sd);

static void inline gar_subdiv_ref(struct gar_subdiv *sd)
{
	gar_subfile_ref(sd->subfile);
	sd->refcnt ++;
}

static void inline gar_subdiv_unref(struct gar_subdiv *sd)
{
	sd->refcnt --;
	if (sd->refcnt == 0)
		gar_free_subdiv_data(sd);
	gar_subfile_unref(sd->subfile);
}

int gar_load_rgnpoints(struct gar_subfile *sub);
int gar_load_rgnpolys(struct gar_subfile *sub);
int gar_load_rgnlines(struct gar_subfile *sub);
