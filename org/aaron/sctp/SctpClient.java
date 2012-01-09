package org.aaron.sctp;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;

import com.sun.nio.sctp.HandlerResult;
import com.sun.nio.sctp.Notification;
import com.sun.nio.sctp.NotificationHandler;

public class SctpClient implements Runnable {

	private final SocketAddress serverAddress;

	private final com.sun.nio.sctp.SctpMultiChannel channel;

	private final ByteBuffer byteBuffer;

	public SctpClient(SocketAddress serverAddress) throws IOException {
		this.serverAddress = serverAddress;
		this.channel = com.sun.nio.sctp.SctpMultiChannel.open();
		System.out
				.println("SCTP_DISABLE_FRAGMENTS = "
						+ this.channel
								.getOption(
										com.sun.nio.sctp.SctpStandardSocketOptions.SCTP_DISABLE_FRAGMENTS,
										null));
		System.out.println("SO_RCVBUF = "
				+ this.channel.getOption(
						com.sun.nio.sctp.SctpStandardSocketOptions.SO_RCVBUF,
						null));
		System.out.println("SO_SNDBUF = "
				+ this.channel.getOption(
						com.sun.nio.sctp.SctpStandardSocketOptions.SO_SNDBUF,
						null));
		this.byteBuffer = ByteBuffer.allocate(2000);
	}

	public void run() {
		try {
			System.out.println("before send");
			channel.send(ByteBuffer.wrap("hello world".getBytes()),
					com.sun.nio.sctp.MessageInfo.createOutgoing(serverAddress,
							0));
			System.out.println("after send");
			while (true) {
				byteBuffer.clear();
				final com.sun.nio.sctp.MessageInfo messageInfo = channel
						.receive(byteBuffer, this,
								new NotificationHandler<SctpClient>() {
									@Override
									public HandlerResult handleNotification(
											Notification notification,
											SctpClient attachment) {
										System.out
												.println("handleNotification notification = "
														+ notification);
										return HandlerResult.CONTINUE;
									}
								});
				System.out.println("after receive, messageInfo = "
						+ messageInfo);
				if ((messageInfo != null) && (messageInfo.isComplete())) {
					System.out.println("byteBuffer = " + byteBuffer);
					final int bytesRead = byteBuffer.position();
					for (int i = 0; i < bytesRead; ++i) {
						System.out.println((char) byteBuffer.get(i));
					}
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			try {
				channel.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	public static void main(String[] args) {
		try {
			new SctpClient(new InetSocketAddress("127.0.0.1", 5000)).run();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

}
