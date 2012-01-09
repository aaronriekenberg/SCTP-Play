package org.aaron.sctp;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;

import com.sun.nio.sctp.HandlerResult;
import com.sun.nio.sctp.Notification;
import com.sun.nio.sctp.NotificationHandler;

public class SctpServer implements Runnable {

	private final com.sun.nio.sctp.SctpMultiChannel channel;

	private final ByteBuffer byteBuffer;

	public SctpServer(SocketAddress listenAddress) throws IOException {
		this.channel = com.sun.nio.sctp.SctpMultiChannel.open();
		this.channel.bind(listenAddress);
		System.out
				.println("SCTP_FRAGMENT_INTERLEAVE = "
						+ this.channel
								.getOption(
										com.sun.nio.sctp.SctpStandardSocketOptions.SCTP_FRAGMENT_INTERLEAVE,
										null));
		this.byteBuffer = ByteBuffer.allocate(20000);
		System.out.println("channel local addresses "
				+ channel.getAllLocalAddresses());
	}

	public void run() {
		while (true) {
			try {
				byteBuffer.clear();
				final com.sun.nio.sctp.MessageInfo messageInfo = channel
						.receive(byteBuffer, null,
								new NotificationHandler<Void>() {
									@Override
									public HandlerResult handleNotification(
											Notification notification,
											Void attachment) {
										System.out
												.println("handleNotification notification = "
														+ notification);
										return HandlerResult.CONTINUE;
									}
								});
				System.out.println("after receive, messageInfo = "
						+ messageInfo);
				if ((messageInfo != null) && (messageInfo.isComplete())) {
					System.out.println("received complete message "
							+ byteBuffer);

					byteBuffer.flip();
					channel.send(byteBuffer, com.sun.nio.sctp.MessageInfo
							.createOutgoing(messageInfo.association(), null, 0));

				}
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}

	public static void main(String[] args) {
		try {
			new SctpServer(new InetSocketAddress(5000)).run();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
