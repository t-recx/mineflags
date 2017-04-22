# guibrella-ish rules for AGUP

AGUP_OBJS := agup.o agtk.o aphoton.o awin95.o aase.o abeos.o ans.o
AGUP_LIB := agup.a

agup_lib: $(AGUP_LIB)

$(AGUP_LIB): $(AGUP_OBJS)
	$(AR) $(ARFLAGS) $@ $^
