#!/usr/bin/env ruby

# Require the Crubyflie gem
require 'crubyflie'
include Crubyflie # easy to use things in namespace

require 'device_input'
require 'pry'

touch = File.open("/dev/input/event0", File::NONBLOCK + File::RDONLY)
Event = DeviceInput::Event

def read_events(io)
  quit = false
  while loop do
    ready = IO::select([io], [], [], 0.1)
    if ready
      bytes = io.read(Event::BYTE_LENGTH)
      break unless (bytes and bytes.length == Event::BYTE_LENGTH)
      data = Event.decode(bytes)
      quit = yield Event.new(data)
    else
      quit = yield nil
    end
  end
end

warn "ready"
x = 0 ; y = 0



# Create a new Crazyflie with cache folder "cache"
cf = Crazyflie.new('cache')
# Before opening any link, scan for interfaces
ifaces = cf.scan_interface
if ifaces.empty?
    logger.error("No crazyflies found")
    exit 1
end
logger.info("Found interfaces: #{ifaces}")

# Open a link to the first interface
cf.open_link(ifaces.first)
# Make sure everything is still good
exit 1 if !cf.active?

# Write the TOCs to stdout
puts "Log TOC"
puts cf.log.toc.to_s
puts
puts "Param TOC"
puts cf.param.toc.to_s

# Read some parameters
puts "--------"
cf.param.get_value("pid_attitude.pitch_kp") do |value|
    puts "kp_pitch: #{value}"
end
cf.param.get_value("pid_attitude.pitch_ki") do |value|
    puts "ki_pitch: #{value}"
end
cf.param.get_value("pid_attitude.pitch_kd") do |value|
    puts "kd_pitch: #{value}"
end
puts "--------"


# We use 1 variable, is_toc = true
# The last two 7 means it is stored and fetched as float
log_conf_var = LogConfVariable.new("stabilizer.pitch", true, 7, 7)

# We create a configuration object
# We want to fetch it every 0.1 secs
log_conf = LogConf.new([log_conf_var], {:period => 10})

# With the configuration object, register a log_block
block_id = cf.log.create_log_block(log_conf)

# Start logging
# Counter on how many times we have logged the pitch
logged = 0
cf.log.start_logging(block_id) do |data|
    warn "Pitch: #{data['stabilizer.pitch']}"
    logged += 1
end

# Wait until we have hit the log_cb 10 times
while (logged < 1)
    sleep 1
end

# Stop logging
cf.log.stop_logging(block_id)

# require 'pry'; binding.pry
def fly(cf, roll, pitch, yawrate, thrust)
  cf.commander.send_setpoint(roll, pitch, yawrate, 10_000 + (50_000 * thrust))
end

# X range is approximately 300 - 3500, smaller numbers are away from me
# (when antenna on left)

read_events(touch) do |e|
  #  warn e
  if e
    if (e.code=='X')
      x = e.value
    end
    if (e.code=='Y')
      y = e.value
    end
    thrust = 1.0 - ((x - 300.0) / 3200.0)
   # warn "x = #{x}\ty = #{y}\tthrust = #{thrust}"
    fly(cf, 0, 0, 0, thrust)
    Kernel.sleep 0.1
  end
  false
end

exit 0
15.times do |t|
  warn t

  sleep 0.1
end

15.times do |t|
  warn t
  commander.send_setpoint(0,0,0, 10001 + 7 * (60_000 - 10_001)/15)
  sleep 0.1
end

15.times do |t|
  warn t
  commander.send_setpoint(0,0,0, 10001 + (15-t) * (60_000 - 10_001)/15)
  sleep 0.1
end

# After finishing, close the link!
cf.close_link()
