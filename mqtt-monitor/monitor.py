#!/usr/bin/env python3

from datetime import datetime
import logging
import requests
import re
import time
import paho.mqtt.client as mqtt
from config import *

MSG_LIMIT_PER_MIN = 20
MSG_LIMIT_PER_HOUR = 50
MSG_LIMIT_PER_DAY = 800

sendCounter = {}
titleCounter = 0


def get_full_class_name(obj):
    module = obj.__class__.__module__
    if module is None or module == str.__class__.__module__:
        return obj.__class__.__name__
    return module + '.' + obj.__class__.__name__


def get_log_filename():
    dt = datetime.now().strftime("%Y%m%d")
    return '/tmp/mqtt-monitor-{}.log'.format(dt)


def logging_config():
    logging.basicConfig(level=logging.INFO,
                        format='[%(asctime)s][%(levelname)s] %(message)s',
                        datefmt='%m%d_%H%M%S',
                        filename=get_log_filename(),
                        filemode='a')
    console = logging.StreamHandler()
    console.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        '[%(asctime)s][%(name)s][%(levelname)s] %(message)s')
    console.setFormatter(formatter)
    logging.getLogger('').addHandler(console)


logging_config()
logger = logging.getLogger("monitor")


def send_ios_report(data):
    try:
        url = BARK_REPORT_URL.format(data['text'], data['desp'])
        r = requests.post(url, timeout=10)
        if r.ok:
            logger.debug("iOS Sent: %s", data["text"])
    except Exception as e:
        logger.warning("iOS Send report failed: %s", get_full_class_name(e))


def send_report(sender, msg):
    global sendCounter
    global titleCounter
    now = datetime.now()
    min_key = now.strftime("Min:%Y-%m-%d %H:%M")
    day_key = now.strftime("Day:%Y-%m-%d")
    min_value = sendCounter.get(min_key, 0)
    day_value = sendCounter.get(day_key, 0)
    if min_value > MSG_LIMIT_PER_MIN:
        logger.warning(
            'Report exceed limits: {} = {}'.format(min_key, min_value))
        return
    if day_value > MSG_LIMIT_PER_DAY:
        logger.warning(
            'Report exceed limits: {} = {}'.format(day_key, day_value))
        return
    sendCounter[min_key] = min_value + 1
    sendCounter[day_key] = day_value + 1
    titleCounter += 1
    suffix = now.strftime("%Y%m%d")
    msg += '\nCreated At '
    msg += now.strftime("%Y-%m-%d %H:%M:%S")
    data = {
        "text": "Device_{}_{}_{}".format(sender, suffix, titleCounter),
        "desp": msg.replace("\n", "  \n"),
    }
    try:
        r = requests.post(WX_REPORT_URL, data=data, timeout=10)
        if r.ok and 'pushid' in r.text:
            logger.info("Sent: [%s]", data["text"])
            logger.debug("Send report to %s successful", sender)
        else:

            logger.warning("Send report to %s failed, status: %s",
                           sender, r.status_code)
    except Exception as e:
        logger.warning("Send report to %s failed: %s",
                       sender, get_full_class_name(e))
    time.sleep(1)
    send_ios_report(data)


def on_message(client, userdata, msg):
    topic = msg.topic
    message = msg.payload.decode('utf8')
    logMsg = "[{}]:<{}> ({},{})".format(topic, message, msg.qos, msg.retain)
    logger.debug(logMsg)
    if topic == 'device/monitor/status':
        return
    m = re.match(r"^device/(\S+)/status$", topic)
    if m and m.group(1):
        send_report(m.group(1), message)


def on_connect(client, userdata, flags, rc):
    logger.info("Connected with result: " + mqtt.error_string(rc))
    # client.subscribe("$SYS/#")
    client.subscribe("#")
    client.publish("device/monitor/status", "Online", retain=True)


def on_disconnect(client, userdata, rc):
    logger.info("Disconnected with result: " + mqtt.error_string(rc))


def create_client():
    client = mqtt.Client(client_id=MQTT_CLIENT_ID, clean_session=True)
    # client.enable_logger()
    # client.on_log = on_log
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    client.username_pw_set(MQTT_USER, MQTT_PASS)
    client.will_set("device/monitor/status", payload="Offline", retain=True)
    client.connect(MQTT_SERVER, port=MQTT_PORT, keepalive=60)
    return client


if __name__ == "__main__":
    client = create_client()
    logger.info("====================")
    logger.info("MQTT Monitor Started")
    client.loop_forever()