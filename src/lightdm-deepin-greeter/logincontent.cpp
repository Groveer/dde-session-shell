/*
 * Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     justforlxz <justforlxz@outlook.com>
 *
 * Maintainer: justforlxz <justforlxz@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "logincontent.h"
#include "logintipswindow.h"
#include "sessionmanager.h"
#include "controlwidget.h"
#include "resetpasswdwidget.h"

#include <DFloatingMessage>
#include <DMessageManager>

DWIDGET_USE_NAMESPACE

LoginContent::LoginContent(SessionBaseModel *const model, QWidget *parent)
    : LockContent(model, parent)
{
    setAccessibleName("LoginContent");

    SessionManager::Reference().setModel(model);
    m_controlWidget->setSessionSwitchEnable(SessionManager::Reference().sessionCount() > 1);
    m_controlWidget->chooseToSession(model->sessionKey());

    m_loginTipsWindow = new LoginTipsWindow;
    connect(m_loginTipsWindow, &LoginTipsWindow::requestClosed, m_model, &SessionBaseModel::tipsShowed);

    connect(m_controlWidget, &ControlWidget::requestSwitchSession, [ = ](QString session) {
        m_model->setCurrentModeState(SessionBaseModel::ModeStatus::SessionMode);
        SessionManager::Reference().switchSession(session);
    });
    connect(m_model, &SessionBaseModel::onSessionKeyChanged, m_controlWidget, &ControlWidget::chooseToSession);
    connect(m_model, &SessionBaseModel::onSessionKeyChanged, this, &LockContent::restoreMode);
    connect(m_model, &SessionBaseModel::tipsShowed, this, &LoginContent::popTipsFrame);
}

void LoginContent::onCurrentUserChanged(std::shared_ptr<User> user)
{
    if (!user.get())
        return;

    LockContent::onCurrentUserChanged(user);
    SessionManager::Reference().updateSession(user->name());
}

void LoginContent::onStatusChanged(SessionBaseModel::ModeStatus status)
{
    switch (status) {
    case SessionBaseModel::ModeStatus::ResetPasswdMode:
        pushChangePasswordFrame();
        break;
    default:
        pushTipsFrame();
    }
}

void LoginContent::pushTipsFrame()
{
    // 如果配置文件存在，并且获取到的内容有效，则显示提示界面，并保证只在greeter启动时显示一次
    if (m_showTipsWidget && m_loginTipsWindow->isValid()) {
        setTopFrameVisible(false);
        setBottomFrameVisible(false);
        QSize size = getCenterContentSize();
        m_loginTipsWindow->setFixedSize(size);
        setCenterContent(m_loginTipsWindow);
    } else {
        LockContent::onStatusChanged(m_model->currentModeState());
    }
}

void LoginContent::popTipsFrame()
{
    // 点击确认按钮后显示登录界面
    m_showTipsWidget = false;
    setTopFrameVisible(true);
    setBottomFrameVisible(true);
    LockContent::onStatusChanged(m_model->currentModeState());
}

void LoginContent::pushChangePasswordFrame()
{
    m_resetPasswordWidget.reset(new ResetPasswdWidget(m_model->currentUser(), this));
    connect(m_resetPasswordWidget.get(), &ResetPasswdWidget::changePasswordSuccessed, this, [ = ] {
        DFloatingMessage *message = new DFloatingMessage(DFloatingMessage::MessageType::TransientType);
        QPalette pa;
        pa.setColor(QPalette::Background, QColor(247, 247, 247, 51));
        message->setPalette(pa);
        message->setIcon(QIcon::fromTheme("dialog-ok"));
        message->setMessage(tr("Password Change Success"));

        message->setAttribute(Qt::WA_DeleteOnClose);
        DMessageManager::instance()->sendMessage(this, message);

        // TODO 这里是回到密码输入界面还是某个因子验证的界面
        m_model->setCurrentModeState(SessionBaseModel::ModeStatus::PasswordMode);
    });
    QSize size = getCenterContentSize();
    m_resetPasswordWidget->setMaximumSize(size);
    setCenterContent(m_resetPasswordWidget.get());

    LockContent::onStatusChanged(m_model->currentModeState());
}
