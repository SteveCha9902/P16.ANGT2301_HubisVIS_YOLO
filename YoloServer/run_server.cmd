@echo off
rem Start HubisVIS post-inspection YOLO server (settings: server_config.json)
cd /d "%~dp0"
python yolo_server.py --config server_config.json
