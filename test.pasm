  loadlib P1, ""
#dlfunc P0, P1, "nci_tB", "tB"
# set S5, "ko\n"
#set_args "0", S5
#invokecc P0
#get_results "0", S5
#print S5
  end
