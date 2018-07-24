import sys

def main():
    if len(sys.argv)!=3:
        print("Usage: python analysis.py <test_name> <args_string>")
        return
    test_name = sys.argv[1]

    stats_file = open(test_name)
    lines = list(map(lambda x: [float(y) for y in x.split("\t")], stats_file.readlines()))

    start_time = lines[0][0]
    end_time = lines[-1][0]

    start_writes_value = lines[0][1]
    end_writes_value = lines[-1][1]

    start_reads_value = lines[0][2]
    end_reads_value = lines[-1][2]

    avg_write_throughput = (end_writes_value - start_writes_value)/(end_time-start_time)
    avg_read_throughput  = (end_reads_value - start_reads_value)/(end_time-start_time)

    max_write_throughput = max([y[1]-x[1] for x,y in zip(lines[:-1], lines[1:])])
    max_read_throughput  = max([y[2]-x[2] for x,y in zip(lines[:-1], lines[1:])])

    print("#writes: {}", lines[-1][1])
    print("#reads : {}", lines[-1][2])

    print("Average writes throughput: {}", avg_write_throughput)
    print("Average reads  throughput: {}", avg_read_throughput)

    print("max writes throughput: {}", max_write_throughput)
    print("max reads  throughput: {}", max_read_throughput )
    return



if __name__ == '__main__':
    main()