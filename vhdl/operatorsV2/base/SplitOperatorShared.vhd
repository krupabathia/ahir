library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library ahir;
use ahir.Types.all;
use ahir.Subprograms.all;
use ahir.Utilities.all;
use ahir.BaseComponents.all;

entity SplitOperatorShared is
    generic
    (
      operator_id   : string;          -- operator id
      input1_is_int : Boolean := true; -- false means float
      input1_characteristic_width : integer := 0; -- characteristic width if input1 is float
      input1_mantissa_width       : integer := 0; -- mantissa width if input1 is float
      iwidth_1      : integer;    -- width of input1
      input2_is_int : Boolean := true; -- false means float
      input2_characteristic_width : integer := 0; -- characteristic width if input2 is float
      input2_mantissa_width       : integer := 0; -- mantissa width if input2 is float
      iwidth_2      : integer;    -- width of input2
      num_inputs    : integer := 2;    -- can be 1 or 2.
      output_is_int : Boolean := true;  -- false means that the output is a float
      output_characteristic_width : integer := 0;
      output_mantissa_width       : integer := 0;
      owidth        : integer;          -- width of output.
      constant_operand : std_logic_vector; -- constant operand.. (it is always the second operand)
      use_constant  : boolean := false;
      zero_delay    : boolean := false;
      no_arbitration: boolean := true;
      num_reqs : integer -- how many requesters?
    );
  port (
    -- req/ack follow level protocol
    reqL                     : in BooleanArray(num_reqs-1 downto 0);
    ackR                     : out BooleanArray(num_reqs-1 downto 0);
    ackL                     : out BooleanArray(num_reqs-1 downto 0);
    reqR                     : in  BooleanArray(num_reqs-1 downto 0);
    -- input data consists of concatenated pairs of ips
    dataL                    : in std_logic_vector(((iwidth_1 + iwidth_2)*num_reqs)-1 downto 0);
    -- output data consists of concatenated pairs of ops.
    dataR                    : out std_logic_vector((owidth*num_reqs)-1 downto 0);
    -- with dataR
    clk, reset              : in std_logic);
end SplitOperatorShared;

architecture Vanilla of SplitOperatorShared is

  constant num_operands : integer := num_inputs;
  constant iwidth : integer := iwidth_1 + iwidth_2;
  
  constant ignore_tag  : boolean := no_arbitration or (reqL'length = 1);

  -- NOTE: the following combination is not allowed
  --       zero_delay = true and ignore_tag = false and reqL'length =1
  --       because it leads to a zero-delay cycle inside the shared operator 
  --
  -- THUS: if an operator is shared by mutually non-exclusive requesters,
  --       (non-compatible operators), then it CANNOT be zero_delay.
  --       This is explicitly blocked out by using the following constant
  constant use_zero_delay : boolean := zero_delay and ((reqL'length = 1) or ignore_tag);
  signal idata : std_logic_vector(iwidth-1 downto 0);
  signal odata: std_logic_vector(owidth-1 downto 0);

  constant tag_length: integer := Maximum(1,Ceil_Log2(reqL'length));
  signal itag,otag : std_logic_vector(tag_length-1 downto 0);
  signal ireq,iack, oreq, oack: std_logic;

  
begin  -- Behave
  assert ackL'length = reqL'length report "mismatched req/ack vectors" severity error;
  
  assert (not zero_delay) or use_zero_delay
    report "Zero delay flag ignored for shared operators which are not exclusive " severity warning;

  assert( (not ((reset = '0') and (clk'event and clk = '1') and no_arbitration)) or Is_At_Most_One_Hot(reqL))
    report "in no-arbitration case, at most one request should be hot on clock edge (in SplitOperatorShared)" severity error;
  
  imux: InputMuxBase
  	generic map(iwidth => iwidth*num_reqs,
	   owidth => iwidth, 
	   twidth => tag_length,
	   nreqs => num_reqs,
	   no_arbitration => no_arbitration)
    port map(
      reqL       => reqL,
      ackL       => ackL,
      reqR       => ireq,
      ackR       => iack,
      dataL      => dataL,
      dataR      => idata,
      tagR       => itag,
      clk        => clk,
      reset      => reset);

  op: SplitOperatorBase
    generic map (
      operator_id   => operator_id,
      input1_is_int => input1_is_int,
      input1_characteristic_width => input1_characteristic_width,
      input1_mantissa_width => input1_mantissa_width,
      iwidth_1  => iwidth_1,
      input2_is_int => input2_is_int,
      input2_characteristic_width => input2_characteristic_width,
      input2_mantissa_width  => input2_mantissa_width,
      iwidth_2  => iwidth_2,
      num_inputs  => num_inputs,
      output_is_int => output_is_int,
      output_characteristic_width => output_characteristic_width,
      output_mantissa_width  => output_mantissa_width,
      owidth    => owidth,
      constant_operand => constant_operand,
      twidth     => tag_length,
      use_constant => use_constant,
      zero_delay  => zero_delay
      )
    port map (
      reqL => ireq,
      ackL => iack,
      reqR => oreq,
      ackR => oack,
      dataL => idata,
      dataR => odata,
      tagR => otag,
      tagL => itag,
      clk => clk,
      reset => reset);


  odemux: OutputDeMuxBase
    generic map (
  	iwidth => owidth,
  	owidth =>  owidth*num_reqs,
	twidth =>  tag_length,
	nreqs  => num_reqs,
	no_arbitration => no_arbitration)
    port map (
      reqL   => oreq,
      ackL   => oack,
      dataL => odata,
      tagL  => otag,
      reqR  => reqR,
      ackR  => ackR,
      dataR => dataR,
      clk   => clk,
      reset => reset);
  
end Vanilla;

