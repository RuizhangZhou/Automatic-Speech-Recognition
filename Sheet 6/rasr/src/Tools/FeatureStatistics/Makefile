#!make

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= feature-statistics$(exe)

FEATURE_STATISTICS_O = \
		$(OBJDIR)/FeatureStatistics.o \
		../../Speech/libSprintSpeech.$(a) \
		../../Search/libSprintSearch.$(a) \
		../../Lattice/libSprintLattice.$(a) \
		../../Lm/libSprintLm.$(a) \
		../../Am/libSprintAm.$(a) \
		../../Mm/libSprintMm.$(a) \
		../../Mc/libSprintMc.$(a) \
		../../Legacy/libSprintLegacy.$(a) \
		../../Bliss/libSprintBliss.$(a) \
		../../Audio/libSprintAudio.$(a) \
		../../Signal/libSprintSignal.$(a) \
		../../Flow/libSprintFlow.$(a) \
		../../Math/libSprintMath.$(a) \
		../../Math/Lapack/libSprintMathLapack.$(a) \
		../../Core/libSprintCore.$(a) \
		../../Fsa/libSprintFsa.$(a)


ifdef MODULE_CART
FEATURE_STATISTICS_O += ../../Cart/libSprintCart.$(a)
endif
ifdef MODULE_FLF_CORE
FEATURE_STATISTICS_O +=	../../Flf/FlfCore/libSprintFlfCore.$(a)
endif


# -----------------------------------------------------------------------------

build: $(TARGETS)

install:
	$(INSTALL) $(TARGETS) $(INSTALL_TARGET)

feature-statistics$(exe): $(FEATURE_STATISTICS_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)
#	$(LD) $(FEATURE_STATISTICS_O) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(patsubst %.o,%.d,$(filter %.o,$(FEATURE_STATISTICS_O)))
