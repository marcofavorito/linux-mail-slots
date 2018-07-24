# AOSV 16/17 project: Linux Mail slots

Project for the course of Advanced Operating Systems and Virtualization (AOSV), A.Y. 2016/2017.

Link to the specifications: [http://www.dis.uniroma1.it/~quaglia/DIDATTICA/AOSV/examination.html](http://www.dis.uniroma1.it/~quaglia/DIDATTICA/AOSV/examination.html)

## How to run

- to mount the module:
    
        sudo ./mailslot_load

as effect, look at `/dev`. You should see `mailslot${minor}` special files. 

- to unmount the module:

		sudo ./mailslot_unload
	

- in order to set the debug mode (i.e. a more auditable module, as you may see with `dmesg`), set `DEBUG` variable to `y`, else `n`.
- for compile:
    
        make
	for run functional tests:
	
		make test
		
- for run performance tests:

		sudo ./perf_eval_scripts/execute_all.sh
		
	sooner or later, the system will crash with 99.999% of probability. Try to tune parameters in both the main script (i.e. `perf_eval_scripts/execute_all.sh`) or in every single test script (should be a `.sh` file). For example, try a lower maximum storage amount.
