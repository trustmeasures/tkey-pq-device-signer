`timescale 1ns/1ps
module replay;
  reg clk;
  always #5 clk = ~clk;
  reg reset_n;
  reg cs;
  reg we;
  reg [7:0] address;
  reg [31:0] write_data;
  wire [31:0] read_data;
  wire ready;
  sha3 dut(clk, reset_n, cs, we, address, write_data, read_data, ready);

  task transaction(input integer wr, input [7:0] addr,
                   input [31:0] value, output [31:0] result);
    begin
      @(posedge clk);
      cs <= 1;
      we <= wr != 0;
      address <= addr;
      write_data <= value;
      wait (ready === 1);
      result = read_data;
      cs <= 0;
      we <= 0;
      wait (ready === 0);
    end
  endtask

  integer fd, op, addr, scan, checks = 0, commands = 0, polls;
  reg [31:0] expected, actual;
  reg [2047:0] path;
  initial begin
    clk = 0;
    reset_n = 0;
    cs = 0;
    we = 0;
    address = 0;
    write_data = 0;
    if (!$value$plusargs("trace=%s", path)) $fatal(1, "missing trace");
    fd = $fopen(path, "r");
    if (!fd) $fatal(1, "cannot open trace");
    repeat (5) @(posedge clk);
    reset_n <= 1;
    repeat (5) @(posedge clk);
    while (!$feof(fd)) begin
      scan = $fscanf(fd, "%h %h %h\n", op, addr, expected);
      if (scan != 3) $fatal(1, "malformed trace");
      transaction(op, addr, expected, actual);
      if (op == 0) begin
        if (actual !== expected)
          $fatal(1, "read mismatch: addr=%h expected=%h actual=%h",
                 addr, expected, actual);
        checks = checks + 1;
      end
      if (op == 1 && addr == 8'h08) begin
        polls = 0;
        actual = 0;
        // Allow the wrapper command to reach the core before polling.
        repeat (3) @(posedge clk);
        while ((actual & 3) != 3) begin
          transaction(0, 8'h09, 0, actual);
          polls = polls + 1;
          if (polls > 10000) $fatal(1, "core timeout");
        end
        commands = commands + 1;
      end
    end
    $fclose(fd);
    $display("PASS: driver MMIO replay against RTL: %0d reads, %0d commands",
             checks, commands);
    $finish;
  end
  initial begin
    #2000000000;
    $fatal(1, "simulation timeout");
  end
endmodule
