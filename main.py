import asyncio
import logging
from aiogram import Bot, Dispatcher, types
from aiogram.filters import Command
from aiogram.methods import DeleteWebhook
from aiogram.types import Message
import requests

# Замените на свои токены
TOKEN = '7596093771:AAEuqTv9cUSuPNXUd_cGbfPsLN8GB-FTtvg'  # 🤖 Токен Telegram-бота
AI_TOKEN = 'io-v2-eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJvd25lciI6ImNlNGU1YTJlLTljNTAtNDkyNy04OTY1LTY1OGJhNzI0MzcxZiIsImV4cCI6NDkwMjM1Nzc4N30.EDhzonH1WO5OhDMK_0GVJk6bn-LqK-ISoXetrD0cbtvhnoFSBNjiimBSMDpLUUSyNRumC6SPVsP8Ep8bDBueAg'  # 🧠 Токен API нейросети

url = "https://api.intelligence.io.solutions/api/v1/chat/completions"
headers = {
    "Content-Type": "application/json",
    "Authorization": f"Bearer {AI_TOKEN}",
}

# Словарь для хранения истории сообщений (простая "память")
user_sessions = {}

logging.basicConfig(level=logging.INFO)
bot = Bot(TOKEN)
dp = Dispatcher()

# 🟢 Обработчик команды /start
@dp.message(Command("start"))
async def cmd_start(message: types.Message):
    user_sessions[message.from_user.id] = []  # Инициализация истории
    await message.answer('👋 алах акбар я бот с ии умею пиздить цепком задавай вопросы ', parse_mode='HTML')

# 🔁 Основной обработчик текста
@dp.message(lambda message: message.text)
async def handle_text(message: Message):
    user_id = message.from_user.id
    user_history = user_sessions.get(user_id, [])

    # Добавляем последнее сообщение пользователя в историю
    user_history.append({"role": "user", "content": message.text})
    if len(user_history) > 10:
        user_history = user_history[-10:]  # Обрезаем до 10 последних сообщений

    data = {
        "model": "mistralai/Mistral-Large-Instruct-2411",
        "messages": [
            {"role": "system", "content": "You are a senior AI software engineer and technical assistant. Write clean, professional code, explain clearly, and help with debugging and architecture."}
        ] + user_history,
    }

    try:
        response = requests.post(url, headers=headers, json=data)
        response.raise_for_status()
        response_json = response.json()

        try:
            text = response_json['choices'][0]['message']['content']
            bot_text = text.split('</think>\n\n')[-1]
        except (KeyError, IndexError):
            bot_text = "⚠️ Ошибка: хуйово вопрос задав давай по новой миша всьо хуйня!."

        # Сохраняем ответ бота в историю
        user_history.append({"role": "assistant", "content": bot_text})
        user_sessions[user_id] = user_history

        await message.answer(bot_text, parse_mode="Markdown")

    except requests.exceptions.RequestException as e:
        logging.error(f"API request failed: {e}")
        await message.answer("⚠️ Ошибка: хуйово ошибка .", parse_mode="Markdown")

# 🖼️ Обработчик изображений
@dp.message(lambda message: message.photo)
async def handle_image(message: types.Message):
    try:
        # Получаем фото с наивысшим разрешением
        file_id = message.photo[-1].file_id
        file = await bot.get_file(file_id)
        file_path = file.file_path
        file_url = f"https://api.telegram.org/file/bot{TOKEN}/{file_path}"

        # Подготавливаем запрос к API
        data = {
            "model": "Qwen/Qwen2-VL-7B-Instruct",  # Убедитесь, что эта модель поддерживается
            "messages": [
                {
                    "role": "system",
                    "content": "You are a senior AI software engineer and technical assistant. Analyze images to identify code, mathematical equations, or technical content, and provide detailed descriptions or insights."
                },
                {
                    "role": "user",
                    "content": [
                        {
                            "type": "text",
                            "text": "Describe this image in detail and provide any technical or visual insights, such as code, equations, or objects write in Ukrainian."
                        },
                        {
                            "type": "image_url",
                            "image_url": {"url": file_url}
                        }
                    ]
                }
            ]
        }

        # Отправляем запрос к API
        response = requests.post(url, headers=headers, json=data)

        # Проверяем успешность запроса
        response.raise_for_status()
        response_json = response.json()

        # Извлекаем текст ответа
        try:
            text = response_json['choices'][0]['message']['content']
        except (KeyError, IndexError):
            text = "⚠️ Ошибка: Не удалось разобрать ответ API. Попробуйте снова."

        await message.answer(text, parse_mode="Markdown")

    except requests.exceptions.RequestException as e:
        logging.error(f"API request failed: {e}")
        await message.answer("⚠️ Ошибка: хуйого сфоткав санечка переснимай.", parse_mode="Markdown")
    except Exception as e:
        logging.error(f"Unexpected error: {e}")
        await message.answer("⚠️ Произошла иа иа пиздец заебав ошыбка иа.", parse_mode="Markdown")

# 🚀 Запуск бота
async def main():
    await bot(DeleteWebhook(drop_pending_updates=True))
    await dp.start_polling(bot)

if __name__ == "__main__":
    asyncio.run(main())
