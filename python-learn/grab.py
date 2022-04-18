#!/usr/bin/python3
# -*- coding: UTF-8 -*-
 
import smtplib
from email.header import Header
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from email.mime.image import MIMEImage

import platform
if platform.system() == 'Linux':
    import pyscreenshot as ImageGrab
else:
    from PIL import ImageGrab # Pillow

from io import StringIO
from io import BytesIO


class GrabScreen(object):
    flag_mail_ssl = False
    mail_port = 25

    def __init__(self, host, user, password, ssl=False, port=25):
        self.flag_mail_ssl = ssl
        self.mail_port = port
        self.mail_user = user
        self.mail_pass = password
        self.mail_host = host

    def gen_mail(self, to, mailtitle, mailcontent):
        msg = MIMEMultipart()
        msg.attach(MIMEText(mailcontent, 'html', 'utf-8'))
        msg['From'] = self.mail_user
        msg['To'] = to
        msg['Subject'] = mailtitle

        im = ImageGrab.grab()
        # memf = StringIO()
        memf = BytesIO()
        im.save(memf, "JPEG")
        msgImage = MIMEImage(memf.getvalue())
        msgImage.add_header('Content-ID', '<img>')
        msg.attach(msgImage)
        return msg

    def send_mail(self, msg, receivers):
        ret = True
        server = None
        if self.flag_mail_ssl:
            server = smtplib.SMTP_SSL(self.mail_host, self.mail_port)
        else:
            server = smtplib.SMTP(self.mail_host, self.mail_port)
        server.ehlo()
        #server.starttls()
        server.login(self.mail_user, self.mail_pass)
        server.sendmail(self.mail_user, receivers, msg.as_string())
        server.quit()
        return ret

    def grab(self, receivers):
        mail_msg = """
<img src='cid:img' title='screen' style='width:100%;height:100%'>"""
        mail = self.gen_mail(','.join(receivers), 'screen', mail_msg)
        self.send_mail(mail, receivers)
        print('done')

if __name__ == '__main__':
    # grab = GrabScreen('smtp.qq.com', '1875803522@qq.com', 'i123madeitABC')
    grab = GrabScreen('smtp.163.com', 'haojunhe@163.com', 'imadeit123ABC')
    # 接收邮件，可设置为你的QQ邮箱或者其他邮箱, e.g. '1875803522@qq.com',  
    # receivers = ['haojunhe@163.com', '1875803522@qq.com']
    receivers = ['haojunhe@163.com']
    grab.grab(receivers)

