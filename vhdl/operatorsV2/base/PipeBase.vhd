library ieee;
use ieee.std_logic_1164.all;

library ahir;
use ahir.Types.all;
use ahir.Subprograms.all;
use ahir.Utilities.all;
use ahir.BaseComponents.all;

entity PipeBase is
  
  generic (num_reads: integer;
           num_writes: integer;
           data_width: integer;
           depth: integer := 1);
  port (
    read_req       : in  std_logic_vector(num_reads-1 downto 0);
    read_ack       : out std_logic_vector(num_reads-1 downto 0);
    read_data      : out std_logic_vector((num_reads*data_width)-1 downto 0);
    write_req       : in  std_logic_vector(num_writes-1 downto 0);
    write_ack       : out std_logic_vector(num_writes-1 downto 0);
    write_data      : in std_logic_vector((num_writes*data_width)-1 downto 0);
    clk, reset : in  std_logic);
  
end PipeBase;

architecture default_arch of PipeBase is

  signal pipe_data, pipe_data_repeated : std_logic_vector(data_width-1 downto 0);
  signal pipe_req, pipe_ack, pipe_req_repeated, pipe_ack_repeated: std_logic;
  
  
begin  -- default_arch

  wmux : OutputPortLevel generic map (
    num_reqs       => num_writes,
    data_width     => data_width,
    no_arbitration => false)
    port map (
      req   => write_req,
      ack   => write_ack,
      data  => write_data,
      oreq  => pipe_req,                -- no cross-over, drives req
      oack  => pipe_ack,                -- no cross-over, receives ack
      odata => pipe_data,
      clk   => clk,
      reset => reset);

  rptr : ShiftRepeaterBase generic map (
    data_width       => data_width,
    number_of_stages => depth)
    port map (
      req_in   => pipe_req,
      ack_out  => pipe_ack,
      data_in  => pipe_data,
      req_out  => pipe_req_repeated,
      ack_in   => pipe_ack_repeated,
      data_out => pipe_data_repeated,
      clk      => clk,
      reset    => reset);

  rmux : InputPortLevel generic map (
    num_reqs       => num_reads,
    data_width     => data_width,
    no_arbitration => false)
    port map (
      req => read_req,
      ack => read_ack,
      data => read_data,
      oreq => pipe_ack_repeated,        -- cross-over, drives ack
      oack => pipe_req_repeated,        -- cross-over, receives req
      odata => pipe_data_repeated,
      clk => clk,
      reset => reset);
  

end default_arch;
