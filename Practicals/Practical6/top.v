//include additional files
`include "alu.v"
`include "regmem.v"
`include "CU.v"

`timescale 1ns / 1ps

module simple_cpu( clk, rst, instruction, epwave0, epwave1, epwave2, epwave3);

    parameter DATA_WIDTH = 8; //8 bit wide data
    parameter ADDR_BITS = 5; //32 Addresses
    parameter INSTR_WIDTH =20; //20b instruction

    input clk, rst;
  	input [INSTR_WIDTH-1:0] instruction;
  	//create outputs to display regfile elements
  	output [DATA_WIDTH-1:0] epwave0;
    output [DATA_WIDTH-1:0] epwave1;
    output [DATA_WIDTH-1:0] epwave2;
    output [DATA_WIDTH-1:0] epwave3;

    //Wires for connecting to data memory    
    wire [ADDR_BITS-1:0] addr_i;
    wire [DATA_WIDTH-1:0] data_in_i, data_out_i, result2_i ;
    wire wen_i; 
    
    //wire for connecting to the ALU
    wire [DATA_WIDTH-1:0]operand_a_i, operand_b_i, result1_i;
    wire [3:0]opcode_i;
    
   
    //Wire for connecting to CU
    wire [DATA_WIDTH-1:0]offset_i;
    wire sel1_i, sel3_i;
  	wire [DATA_WIDTH-1:0] operand_1_i, operand_2_i;
  	wire [DATA_WIDTH-1:0] ep_i [0:3]; //create a wire array for the epwaves for regfile

    
    
    //Instantiating an alu1
    alu #(DATA_WIDTH) alu1 (clk, operand_a_i, operand_b_i, opcode_i, result1_i);
     
    //instantiation of data memory
  	reg_mem  #(DATA_WIDTH,ADDR_BITS) data_memory(addr_i, data_in_i, wen_i, clk, data_out_i);
    
    //Instantiation of a CU
 	 CU  #(DATA_WIDTH,ADDR_BITS, INSTR_WIDTH) CU1(clk, rst, instruction, result2_i, operand_1_i, operand_2_i, offset_i, opcode_i, sel1_i, sel3_i, wen_i, ep_i[0], ep_i[1], ep_i[2], ep_i[3]);

    
    //Connect CU to ALU
    assign operand_a_i = operand_1_i;
    assign operand_b_i = (sel3_i == 0) ? operand_2_i: (sel3_i == 1) ? offset_i : 8'bx;
    
    //Connect CU to Memory
    assign data_in_i = operand_2_i;
    assign epwave0 = ep_i[0];
    assign epwave1 = ep_i[1];
    assign epwave2 = ep_i[2];
    assign epwave3 = ep_i[3];

    //Connect datamem to CU
    assign result2_i = (sel1_i == 0) ? data_out_i : (sel1_i == 1) ? result1_i : 8'bx;  

endmodule
