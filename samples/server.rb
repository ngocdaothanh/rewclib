# Send the Green channel of the webcam image to client.

PORT = 1225
COMPRESS = true  # Compress Green channel

WIDTH  = 640
HEIGHT = 480
#WIDTH  = 320
#HEIGHT = 240
FPS    = 30

require 'socket'
require 'zlib'

dir = File.dirname(__FILE__)
require dir + '/../rewclib'

$done = false
trap(:INT) { $done = true }

server = TCPServer.new(PORT)
while true
  break if $done

  puts "Waiting for client to connect on port #{PORT}..."
  begin
    socket = server.accept
  rescue Errno::EAGAIN, Errno::ECONNABORTED, Errno::EPROTO, Errno::EINTR
    IO.select([server])
    retry
  end
  puts 'Client connected'

  cam = Rewclib.new
  cam.open(WIDTH, HEIGHT, FPS)
  puts 'Camera opened'

  # Send video size as header
  socket.send([WIDTH].pack('I!'), 0)
  socket.send([HEIGHT].pack('I!'), 0)
  socket.send([COMPRESS ? 1 : 0].pack('I!'), 0)

  begin
    while true
      image = cam.image(false, 1)  # Not upsidedown, Green
      if COMPRESS
        compressed_image = Zlib::Deflate.deflate(image)

        # Send the size as header, then the compressed image as body
        socket.send([compressed_image.size].pack('I!'), 0)
        socket.send(compressed_image, 0)
      else
        socket.send(image, 0)
      end
    end
  rescue
  ensure
    cam.close
    socket.close
    puts 'Camera closed'
    puts 'Client disconnected'
  end
end
