void Compiler::run() {
  while (!jobs.is_empty()) {
    Job job = jobs.pop_front();

    switch (job.kind) {
    case Job_read_file: {} break;
    case Job_tokenize: {} break;
    case Job_parse: {
			    b32 ok = parse();
			    // ...
			    // if has imports: add read_file jobs to queue
		    } break;
    case Job_typecheck: {} break;
    }
  }
}
