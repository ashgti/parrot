OPSC_DIR = compilers/opsc

OPSC_CLEANUPS = \
	$(OPSC_DIR)/opsc.pbc \
	$(OPSC_DIR)/gen/Ops/Compiler.pir \
	$(OPSC_DIR)/gen/Ops/Compiler/Actions.pir \
	$(OPSC_DIR)/gen/Ops/Compiler/Grammar.pir \

OPSC_SOURCES = \
	$(OPSC_DIR)/opsc.pir \
	$(OPSC_DIR)/src/builtins.pir \
	$(OPSC_DIR)/gen/Ops/Compiler.pir \
	$(OPSC_DIR)/gen/Ops/Compiler/Actions.pir \
	$(OPSC_DIR)/gen/Ops/Compiler/Grammar.pir \
	\
	$(OPSC_DIR)/gen/Ops/Op.pir
