#!/usr/bin/env ruby

# Require the Crubyflie gem
require 'crubyflie'
include Crubyflie # easy to use things in namespace

require 'device_input'
require 'pry'

require_relative './mpu6050'

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

mp = MPU6050.new('/home/pi/wiringPi/wiringPi/libwiringPi.so.2.44')

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
log_conf = LogConf.new([log_conf_var], {:period => 100})

# With the configuration object, register a log_block
block_id = cf.log.create_log_block(log_conf)

# Start logging
# Counter on how many times we have logged the pitch
logged = 0
cf.log.start_logging(block_id) do |data|
    warn "Value: #{data['stabilizer.pitch']}"
end

def fly(cf, roll, pitch, yawrate, thrust)
  cf.commander.send_setpoint(roll, pitch, yawrate, 10_000 + (45_000 * thrust))
end

# X range is approximately 300 - 3500, smaller numbers are away from me
# (when antenna on left)
cf.commander.send_setpoint(0, 0, 0, 0)

pitch = 0
read_events(touch) do |e|
  thrust = 0
  if e
    if (e.code=='X')
      x = e.value
    end
    if (e.code=='Y')
      y = e.value
    end
    thrust = 1.0 - ((x - 300.0) / 3200.0)
  end
  accel_z = mp.measure
  pitch = (accel_z > 0) ? 10.7 : -10.8
  warn "pitch = #{pitch} thrust = #{thrust}"
  fly(cf, 0.01, pitch, 0.001, thrust)
  Kernel.sleep 0.1
  false
end


# Stop logging
cf.log.stop_logging(block_id)

# After finishing, close the link!
cf.close_link()
