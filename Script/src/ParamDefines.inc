  ParamDefine("database", ParamValue::TypeString),
  ParamDefine("reads", ParamValue::TypeString),
  ParamDefine("psl", ParamValue::TypeString),
  ParamDefine("match_threshold", ParamValue::TypeDouble),
  ParamDefine("match_max_mismatch", ParamValue::TypeInt),
  ParamDefine("filter", ParamValue::TypeString),
  ParamDefine("phylogeny", ParamValue::TypeString),
  ParamDefine("phylogeny_structure_file", ParamValue::TypeString),
  ParamDefine("coverrecs", ParamValue::TypeString),
  ParamDefine("alignment", ParamValue::TypeString),
  ParamDefine("profile", ParamValue::TypeString),
  ParamDefine("preprocess", ParamValue::TypeString),
  ParamDefine("output", ParamValue::TypeString),
  ParamDefine("output_files", ParamValue::TypeList),
  ParamDefine("input", ParamValue::TypeString),
  ParamDefine("readhits", ParamValue::TypeString),
  ParamDefine("readhits_list", ParamValue::TypeString),
  ParamDefine("num_reads", ParamValue::TypeInt),
  ParamDefine("read_mutate_rate", ParamValue::TypeDouble),
  ParamDefine("read_indel_rate", ParamValue::TypeDouble),
  ParamDefine("read_homopoly_rate", ParamValue::TypeDouble),
  ParamDefine("read_degradation_len", ParamValue::TypeDouble),
  ParamDefine("read_length_avg", ParamValue::TypeDouble),
  ParamDefine("read_length_std", ParamValue::TypeDouble),
  ParamDefine("read_length_min", ParamValue::TypeInt),
  ParamDefine("read_length_max", ParamValue::TypeInt),
  ParamDefine("read_alignment_start", ParamValue::TypeInt),
  ParamDefine("read_alignment_end", ParamValue::TypeInt),
  ParamDefine("threshold", ParamValue::TypeDouble),
  ParamDefine("primer_length", ParamValue::TypeInt),
  ParamDefine("primer_conservation_threshold", ParamValue::TypeDouble),
  ParamDefine("multiplicity_threshold", ParamValue::TypeDouble),
  ParamDefine("represented_threshold", ParamValue::TypeDouble),
  ParamDefine("reference", ParamValue::TypeString),

  ParamDefine("rand_seed", ParamValue::TypeInt),
  ParamDefine("max_mem_mb", ParamValue::TypeInt),

  ParamDefine("parallel", ParamValue::TypeString), // init,runjob,check,done
  ParamDefine("parallel_info_file", ParamValue::TypeString),
  ParamDefine("parallel_job", ParamValue::TypeInt),
  ParamDefine("parallel_num_jobs", ParamValue::TypeInt),
  ParamDefine("parallel_run_list", ParamValue::TypeString),
  ParamDefine("parallel_max_job_seconds", ParamValue::TypeInt),
